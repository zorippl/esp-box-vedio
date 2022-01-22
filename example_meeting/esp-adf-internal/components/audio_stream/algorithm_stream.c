/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2019 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "audio_element.h"
#include "audio_error.h"
#include "audio_mem.h"
#include "audio_thread.h"
#include "esp_log.h"

#include "algorithm_stream.h"
#include "esp_afe_sr_iface.h"
#include "esp_afe_sr_models.h"

// #define DEBUG_ALGO_INPUT // debug input AFE feed data
#define AEC_FRAME_BYTES                 (1024) // 32ms data frame (32 * 16 * 2)
#define ALGORITHM_FETCH_TASK_STACK_SIZE (2 * 1024)

static const char *TAG = "ALGORITHM_STREAM";

const int FETCH_STOPPED_BIT = BIT0;

typedef struct {
    char *scale_buff;
    int16_t *div_buff;
    int16_t *aec_buff;
    int8_t algo_mask;
    bool afe_fetch_run;
    algorithm_stream_input_type_t input_type;
    const esp_afe_sr_iface_t *afe_handle;
    esp_afe_sr_data_t *afe_data;
    EventGroupHandle_t state;
} algo_stream_t;

char *get_model_base_path(void)
{
    return NULL;
}

static esp_err_t _algo_close(audio_element_handle_t self)
{
    algo_stream_t *algo = (algo_stream_t *)audio_element_getdata(self);

    algo->afe_fetch_run = false;

    if (algo->afe_data) {
        algo->afe_handle->destroy(algo->afe_data);
        algo->afe_data = NULL;
    }

    xEventGroupWaitBits(algo->state, FETCH_STOPPED_BIT, false, true, portMAX_DELAY);

    if (algo->aec_buff) {
        audio_free(algo->aec_buff);
        algo->aec_buff = NULL;
    }

    if (algo->scale_buff) {
        audio_free(algo->scale_buff);
        algo->scale_buff = NULL;
    }

    if (algo->div_buff) {
        audio_free(algo->div_buff);
        algo->div_buff = NULL;
    }

    if (algo) {
        audio_free(algo);
        algo = NULL;
    }

    return ESP_OK;
}

void _algo_fetch_task(void *pv)
{
    audio_element_handle_t self = pv;
    algo_stream_t *algo = (algo_stream_t *)audio_element_getdata(self);

    while (algo->afe_fetch_run && algo->afe_data) {
        algo->afe_handle->fetch(algo->afe_data, algo->aec_buff);
        audio_element_output(self, (char *)algo->aec_buff, AEC_FRAME_BYTES);
    }

    xEventGroupSetBits(algo->state, FETCH_STOPPED_BIT);
    vTaskDelete(NULL);
}

static esp_err_t _algo_open(audio_element_handle_t self)
{
    algo_stream_t *algo = (algo_stream_t *)audio_element_getdata(self);
    bool _success = true;

    algo->afe_handle = &esp_afe_sr_1mic;
    afe_config_t afe_config = AFE_CONFIG_DEFAULT();
    afe_config.se_init = true;
    afe_config.vad_init = false;
    afe_config.wakenet_init = false;
    afe_config.afe_perferred_core = 1;
    afe_config.wakenet_mode = DET_MODE_90;
    algo->afe_data = algo->afe_handle->create_from_config(&afe_config);
    algo->afe_fetch_run = true;

    xEventGroupClearBits(algo->state, FETCH_STOPPED_BIT);
#ifndef DEBUG_ALGO_INPUT
    audio_thread_create(NULL, "algo_fetch", _algo_fetch_task, (void *)self, ALGORITHM_FETCH_TASK_STACK_SIZE,
        ALGORITHM_STREAM_TASK_PERIOD, false, ALGORITHM_STREAM_PINNED_TO_CORE);
#else
    xEventGroupSetBits(algo->state, FETCH_STOPPED_BIT);
#endif

    AUDIO_NULL_CHECK(TAG, _success, {
        _algo_close(self);
        return ESP_FAIL;
    });
    return ESP_OK;
}

static esp_err_t algorithm_data_gain(int16_t *raw_buff, int len, int linear_lfac, int linear_rfac)
{
    for (int i = 0; i < len / 4; i++) {
        raw_buff[i << 1] = raw_buff[i << 1] * linear_lfac;
        raw_buff[(i << 1) + 1] = raw_buff[(i << 1) + 1] * linear_rfac;
    }
    return ESP_OK;
}

static esp_err_t algorithm_data_divided(int16_t *raw_buff, int len, int16_t *left_channel, int linear_lfac, int16_t *right_channel, int linear_rfac)
{
    // To improve efficiency, data splitting and linear amplification are integrated into one function
    for (int i = 0; i < len / 4; i++) {
        if (left_channel) {
            left_channel[i] = raw_buff[i << 1] * linear_lfac;
        }
        if (right_channel) {
            right_channel[i] = raw_buff[(i << 1) + 1] * linear_rfac;
        }
    }
    return ESP_OK;
}

static int algorithm_data_process_for_type1(audio_element_handle_t self)
{
    algo_stream_t *algo = (algo_stream_t *)audio_element_getdata(self);
    int in_ret = 0;

    in_ret = audio_element_input(self, algo->scale_buff, 2 * AEC_FRAME_BYTES);
    if (in_ret > 0) {
#ifndef DEBUG_ALGO_INPUT
        algorithm_data_gain((int16_t *)algo->scale_buff, 2 * AEC_FRAME_BYTES, 1, 3);
        algo->afe_handle->feed(algo->afe_data, (int16_t *)algo->scale_buff);
#else
        audio_element_output(self, algo->scale_buff, 2 * AEC_FRAME_BYTES);
#endif
    }

    return in_ret;
}

static int algorithm_data_process_for_type2(audio_element_handle_t self)
{
    algo_stream_t *algo = (algo_stream_t *)audio_element_getdata(self);
    int in_ret = 0;
    char *record = audio_calloc(1, AEC_FRAME_BYTES);
    char *reference = audio_calloc(1, AEC_FRAME_BYTES);

    in_ret = audio_element_input(self, record, AEC_FRAME_BYTES);
    in_ret |= audio_element_multi_input(self, (char *)algo->div_buff, 2 * AEC_FRAME_BYTES, 0, portMAX_DELAY);
    if (in_ret > 0) {
        int16_t *temp = (int16_t *)algo->scale_buff;
        algorithm_data_divided(algo->div_buff, 2 * AEC_FRAME_BYTES, (int16_t *)reference, 1, NULL, 0);
        for (int i = 0; i < (AEC_FRAME_BYTES / 2); i++) {
            temp[i << 1] = ((int16_t *)record)[i];
            temp[(i << 1) + 1] = ((int16_t *)reference)[i];
        }
#ifndef DEBUG_ALGO_INPUT
        algo->afe_handle->feed(algo->afe_data, temp);
#else
        audio_element_output(self, temp, 2 * AEC_FRAME_BYTES);
#endif
    }
    audio_free(record);
    audio_free(reference);
    return in_ret;
}

static audio_element_err_t _algo_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    int ret = ESP_OK;
    algo_stream_t *algo = (algo_stream_t *)audio_element_getdata(self);

    if (algo->input_type == ALGORITHM_STREAM_INPUT_TYPE1) {
        ret = algorithm_data_process_for_type1(self);
    } else if (algo->input_type == ALGORITHM_STREAM_INPUT_TYPE2) {
        ret = algorithm_data_process_for_type2(self);
    } else {
        ESP_LOGE(TAG, "Type %d is not supported", algo->input_type);
        return AEL_IO_FAIL;
    }
    return ret;
}

audio_element_handle_t algo_stream_init(algorithm_stream_cfg_t *config)
{
    AUDIO_NULL_CHECK(TAG, config, return NULL);
    if ((config->rec_linear_factor <= 0) || (config->ref_linear_factor <= 0)) {
        ESP_LOGE(TAG, "The linear amplication factor should be greater than 0");
        return NULL;
    }

    algo_stream_t *algo = (algo_stream_t *)audio_calloc(1, sizeof(algo_stream_t));
    AUDIO_NULL_CHECK(TAG, algo, return NULL);

    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.open = _algo_open;
    cfg.close = _algo_close;
    cfg.process = _algo_process;
    cfg.task_stack = config->task_stack;
    cfg.task_prio = config->task_prio;
    cfg.task_core = config->task_core;
    cfg.multi_in_rb_num = config->input_type;
    cfg.tag = "algorithm";

    cfg.buffer_len = AEC_FRAME_BYTES;
    algo->input_type = config->input_type;
    algo->algo_mask = config->algo_mask;
    algo->state = xEventGroupCreate();
    audio_element_handle_t el = audio_element_init(&cfg);
    AUDIO_NULL_CHECK(TAG, el, {
        audio_free(algo);
        return NULL;
    });
    bool _success = true;
    _success &= ((algo->scale_buff = audio_calloc(1, 2 * AEC_FRAME_BYTES)) != NULL);
    if (algo->algo_mask & ALGORITHM_STREAM_USE_AEC) {
        _success &= ((algo->aec_buff = audio_calloc(1, AEC_FRAME_BYTES)) != NULL);
    }

    if (algo->input_type == ALGORITHM_STREAM_INPUT_TYPE2) {
        _success &= ((algo->div_buff = audio_calloc(1, 2 * AEC_FRAME_BYTES)) != NULL);
    }

    AUDIO_NULL_CHECK(TAG, _success, {
        ESP_LOGE(TAG, "Error occured");
        _algo_close(el);
        audio_free(algo);
        return NULL;
    });

    audio_element_setdata(el, algo);
    return el;
}

ringbuf_handle_t algo_stream_get_multi_input_rb(audio_element_handle_t algo_handle)
{
    AUDIO_NULL_CHECK(TAG, algo_handle, return NULL);
    return audio_element_get_multi_input_ringbuf(algo_handle, 0);
}

esp_err_t algo_stream_set_record_rate(audio_element_handle_t algo_handle, int rec_ch, int rec_sample_rate)
{
    return ESP_OK;
}

esp_err_t algo_stream_set_reference_rate(audio_element_handle_t algo_handle, int ref_ch, int ref_sample_rate)
{
    return ESP_OK;
}
