
/**
  ******************************************************************************
  * @file    app_x-cube-ai.c
  * @author  X-CUBE-AI C code generator
  * @brief   AI program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#if defined ( __ICCARM__ )
#elif defined ( __CC_ARM ) || ( __GNUC__ )
#endif

/* System headers */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#include "app_x-cube-ai.h"
#include "bsp_ai.h"
#include "aiSystemPerformance.h"
#include "ai_datatypes_defines.h"

/* USER CODE BEGIN includes */
//#define MANUAL_MODE
#define LIVE_MODE

 extern UART_HandleTypeDef huart3;


 static ai_handle network = AI_HANDLE_NULL;
 static ai_buffer *ai_input;
 static ai_buffer *ai_output;
 extern ADC_HandleTypeDef hadc1;

 void Read_All_Sensors(float *mq2,
                       float *mq3,
                       float *mq5,
                       float *mq8,
                       float *mq135);
 static char msg[400];

 /* manual test values */
 float mq2 = 70;
 float mq3 = 243;
 float mq5 = 65;
 float mq8 = 172;
 float mq135 = 921;

 /* scaler values from Python */
 float mean_vals[5] = {
     134.63149876f,
     529.60772062f,
     327.79090987f,
     585.58406231f,
     1813.49026791f
 };

 float std_vals[5] = {
     72.74553803f,
     278.31779565f,
     256.96472218f,
     350.74140164f,
     847.7146359f
 };
/* USER CODE END includes */

/* IO buffers ----------------------------------------------------------------*/

DEF_DATA_IN

DEF_DATA_OUT
/* Activations buffers -------------------------------------------------------*/

AI_ALIGNED(32)
static uint8_t pool0[AI_NETWORK_DATA_ACTIVATION_1_SIZE];

ai_handle data_activations0[] = {pool0};

/* Entry points --------------------------------------------------------------*/

void MX_X_CUBE_AI_Init(void)
{
    ai_error err;
    ai_handle net;
    ai_network_report report;

    const char* nn_name = ai_mnetwork_find(NULL, 0);

    err = ai_mnetwork_create(nn_name, &net, NULL);

    if (err.type != AI_ERROR_NONE)
    {
        sprintf(msg,"Create error: %d %d\r\n", err.type, err.code);
        HAL_UART_Transmit(&huart3,(uint8_t*)msg,strlen(msg),HAL_MAX_DELAY);
        return;
    }

    network = net;

    if (!ai_mnetwork_init(network))
    {
        sprintf(msg,"Init failed\r\n");
        HAL_UART_Transmit(&huart3,(uint8_t*)msg,strlen(msg),HAL_MAX_DELAY);
        return;
    }

    ai_handle net_handle;
    ai_network_params params;

    ai_mnetwork_get_private_handle(network,&net_handle,&params);

    ai_input = ai_network_inputs_get(net_handle,NULL);
    ai_output = ai_network_outputs_get(net_handle,NULL);

    if (ai_mnetwork_get_report(network,&report))
    {
        sprintf(msg,
                "Input size: %d Output size: %d\r\n",
                (int)report.inputs[0].size,
                (int)report.outputs[0].size);

        HAL_UART_Transmit(&huart3,(uint8_t*)msg,strlen(msg),HAL_MAX_DELAY);
    }

    sprintf(msg,"AI INIT SUCCESS\r\n");
    HAL_UART_Transmit(&huart3,(uint8_t*)msg,strlen(msg),HAL_MAX_DELAY);
}
#ifdef LIVE_MODE
void MX_X_CUBE_AI_Process(void)
{
    static uint8_t calibration_done = 0;

    static float mq2_base = 0;
    static float mq3_base = 0;
    static float mq5_base = 0;
    static float mq8_base = 0;
    static float mq135_base = 0;

    static ai_i8 input_data[5];
    static ai_i8 output_data[1];

    float live_mq2, live_mq3, live_mq5, live_mq8, live_mq135;

    float adapted[5];
    float normalized[5];

    //---------------------------------------------------
    // STEP 1: warmup sensors
    //---------------------------------------------------
    if(calibration_done == 0)
    {
        sprintf(msg,"\r\nWaiting sensor stabilization...\r\n");
        HAL_UART_Transmit(&huart3,(uint8_t*)msg,strlen(msg),HAL_MAX_DELAY);

        for(int i=30;i>=0;i--)
        {
            sprintf(msg,"%d\r\n",i);
            HAL_UART_Transmit(&huart3,(uint8_t*)msg,strlen(msg),HAL_MAX_DELAY);
            HAL_Delay(1000);
        }

        //---------------------------------------------------
        // STEP 2: collect clean-air baseline
        //---------------------------------------------------
        sprintf(msg,
        "\r\nPut sensors in clean air area\r\n"
        "Collecting baseline for 10 seconds...\r\n");

        HAL_UART_Transmit(&huart3,(uint8_t*)msg,strlen(msg),HAL_MAX_DELAY);

        float sum2=0,sum3=0,sum5=0,sum8=0,sum135=0;

        for(int i=0;i<10;i++)
        {
            Read_All_Sensors(
                &live_mq2,
                &live_mq3,
                &live_mq5,
                &live_mq8,
                &live_mq135
            );

            sum2 += live_mq2;
            sum3 += live_mq3;
            sum5 += live_mq5;
            sum8 += live_mq8;
            sum135 += live_mq135;

            sprintf(msg,"Baseline sample %d/10\r\n",i+1);
            HAL_UART_Transmit(&huart3,(uint8_t*)msg,strlen(msg),HAL_MAX_DELAY);

            HAL_Delay(1000);
        }

        mq2_base = sum2/10.0f;
        mq3_base = sum3/10.0f;
        mq5_base = sum5/10.0f;
        mq8_base = sum8/10.0f;
        mq135_base = sum135/10.0f;

        sprintf(msg,
        "\r\nBASE VALUES:\r\n"
        "MQ2: %.2f\r\n"
        "MQ3: %.2f\r\n"
        "MQ5: %.2f\r\n"
        "MQ8: %.2f\r\n"
        "MQ135: %.2f\r\n",
        mq2_base,mq3_base,mq5_base,mq8_base,mq135_base);

        HAL_UART_Transmit(&huart3,(uint8_t*)msg,strlen(msg),HAL_MAX_DELAY);

        sprintf(msg,"\r\nNow insert fruit...\r\n");
        HAL_UART_Transmit(&huart3,(uint8_t*)msg,strlen(msg),HAL_MAX_DELAY);

        calibration_done = 1;
        return;
    }

    //---------------------------------------------------
    // STEP 3: read live sensors
    //---------------------------------------------------
    Read_All_Sensors(
        &live_mq2,
        &live_mq3,
        &live_mq5,
        &live_mq8,
        &live_mq135
    );

    //---------------------------------------------------
    // STEP 4: calculate delta
    //---------------------------------------------------
    /* ===================================================
       STEP 4: DELTA-BASED MAPPING (NEW METHOD)
       =================================================== */

    /* avoid division logic completely */
    float delta2   = live_mq2   - mq2_base;
    float delta3   = live_mq3   - mq3_base;
    float delta5   = live_mq5   - mq5_base;
    float delta8   = live_mq8   - mq8_base;
    float delta135 = live_mq135 - mq135_base;
    if(delta2 < 0) delta2 = 0;
    if(delta3 < 0) delta3 = 0;
    if(delta5 < 0) delta5 = 0;
    if(delta8 < 0) delta8 = 0;
    if(delta135 < 0) delta135 = 0;

    /* ===================================================
       STEP 5: MAP BASELINE SENSOR → DATASET BASELINE
       then ADD DELTA
       =================================================== */

    /* dataset baseline values (from training distribution) */
    const float ds_base2   = 50;
    const float ds_base3   = 120;
    const float ds_base5   = 35;
    const float ds_base8   = 100;
    const float ds_base135 = 800;

    /* apply your new logic */
    adapted[0] = ds_base2   + delta2;
    adapted[1] = ds_base3   + delta3+(delta2*0.5f);
    adapted[2] = ds_base5   + delta5;
    adapted[3] = ds_base8   + delta8;
    adapted[4] = ds_base135 + delta135+delta2;
//    adapted[0] = 70;
//    adapted[1] = 240;
//    adapted[2] = 65;
//    adapted[3] = 170;
//    adapted[4] = 920;

    /* optional safety clipping (IMPORTANT for STM32 stability) */
    if(adapted[0] < 0) adapted[0] = 0;
    if(adapted[1] < 0) adapted[1] = 0;
    if(adapted[2] < 0) adapted[2] = 0;
    if(adapted[3] < 0) adapted[3] = 0;
    if(adapted[4] < 0) adapted[4] = 0;

    if(adapted[0] > 250) adapted[0] = 250;
    if(adapted[1] > 800) adapted[1] = 800;
    if(adapted[2] > 770) adapted[2] = 770;
    if(adapted[3] > 1000) adapted[3] = 1000;
    if(adapted[4] > 3000) adapted[4] = 3000;

    //---------------------------------------------------
    // STEP 6: normalize
    //---------------------------------------------------
    for(int i=0;i<5;i++)
    {
        normalized[i] =
        (adapted[i] - mean_vals[i]) / std_vals[i];
    }

    //---------------------------------------------------
    // STEP 7: quantize
    //---------------------------------------------------
    float input_scale = 0.015845218673348427f;
    int input_zero_point = -25;

    for(int i=0;i<5;i++)
    {
        float q =
        (normalized[i]/input_scale)
        + input_zero_point;

        if(q > 127) q = 127;
        if(q < -128) q = -128;

        input_data[i] = (ai_i8)q;
    }

    ai_input[0].data = AI_HANDLE_PTR(input_data);
    ai_output[0].data = AI_HANDLE_PTR(output_data);

    ai_i32 batch =
    ai_mnetwork_run(network, ai_input, ai_output);

    if(batch != 1)
    {
        sprintf(msg,"Inference failed\r\n");
        HAL_UART_Transmit(&huart3,(uint8_t*)msg,strlen(msg),HAL_MAX_DELAY);
        return;
    }

    //---------------------------------------------------
    // STEP 8: output
    //---------------------------------------------------
    int8_t raw_output = output_data[0];

    float prediction =
    ((raw_output + 128) * 0.00390625f);

    const char* result;
    float confidence;

    if(prediction > 0.5f)
    {
        result = "ROTTEN";
        confidence = prediction*100;
    }
    else
    {
        result = "FRESH";
        confidence = (1-prediction)*100;
    }

    sprintf(msg,
    "\r\n============================\r\n"
    "LIVE READINGS:\r\n"
    "MQ2: %.2f\r\n"
    "MQ3: %.2f\r\n"
    "MQ5: %.2f\r\n"
    "MQ8: %.2f\r\n"
    "MQ135: %.2f\r\n"
    "\r\n"
    "ADAPTED VALUES:\r\n"
    "MQ2: %.2f\r\n"
    "MQ3: %.2f\r\n"
    "MQ5: %.2f\r\n"
    "MQ8: %.2f\r\n"
    "MQ135: %.2f\r\n"
    "\r\n"
    "RESULT: %s\r\n"
    "CONFIDENCE: %.2f%%\r\n"
    "============================\r\n",
    live_mq2,live_mq3,live_mq5,live_mq8,live_mq135,
    adapted[0],adapted[1],adapted[2],adapted[3],adapted[4],
    result,
    confidence);

    HAL_UART_Transmit(&huart3,
                      (uint8_t*)msg,
                      strlen(msg),
                      HAL_MAX_DELAY);
    sprintf(msg,
    "Q INPUTS: %d %d %d %d %d\r\n",
    input_data[0],
    input_data[1],
    input_data[2],
    input_data[3],
    input_data[4]);

    HAL_UART_Transmit(&huart3,
                      (uint8_t*)msg,
                      strlen(msg),
                      HAL_MAX_DELAY);
}
#endif
extern ADC_HandleTypeDef hadc1;


#ifdef MANUAL_MODE

void MX_X_CUBE_AI_Process(void)
{
    static ai_i8 input_data[5];
    static ai_i8 output_data[1];

    /* same normalization used in Python */
    float normalized[5];

    float raw[5] = {mq2, mq3, mq5, mq8, mq135};

    for(int i=0;i<5;i++)
    {
        normalized[i] = (raw[i] - mean_vals[i]) / std_vals[i];
    }

    /*
    Convert float normalized values to int8
    X-CUBE-AI usually expects int8 range
    */

    float input_scale = 0.015845218673348427f;
    int input_zero_point = -25;

    for(int i=0; i<5; i++)
    {
        float q = (normalized[i] / input_scale) + input_zero_point;

        if(q > 127)
            q = 127;

        if(q < -128)
            q = -128;

        input_data[i] = (ai_i8)q;
    }

    ai_input[0].data = AI_HANDLE_PTR(input_data);
    ai_output[0].data = AI_HANDLE_PTR(output_data);

    ai_i32 batch = ai_mnetwork_run(network, ai_input, ai_output);

    if(batch != 1)
    {
        sprintf(msg,"Inference failed\r\n");
    }
    else
    {
    	int8_t raw_output = output_data[0];

    	/* dequantize output */
    	float output_scale = 0.00390625f;
    	int output_zero_point = -128;

    	float prediction =
    	((float)(raw_output - output_zero_point)) * output_scale;

    	/* classify */
    	const char* result;
    	float confidence;

    	if(prediction > 0.5f)
    	{
    	    result = "ROTTEN";
    	    confidence = prediction * 100.0f;
    	}
    	else
    	{
    	    result = "FRESH";
    	    confidence = (1.0f - prediction) * 100.0f;
    	}

    	/* print everything clearly */
    	sprintf(msg,
    	"\r\n==============================\r\n"
    	"SENSOR INPUTS:\r\n"
    	"MQ2   : %.2f\r\n"
    	"MQ3   : %.2f\r\n"
    	"MQ5   : %.2f\r\n"
    	"MQ8   : %.2f\r\n"
    	"MQ135 : %.2f\r\n"
    	"\r\n"
    	"MODEL OUTPUT:\r\n"
    	"Raw INT8   : %d\r\n"
    	"Probability : %.4f\r\n"
    	"\r\n"
    	"FINAL RESULT:\r\n"
    	"Class      : %s\r\n"
    	"Confidence : %.2f %%\r\n"
    	"==============================\r\n",
    	mq2, mq3, mq5, mq8, mq135,
    	raw_output,
    	prediction,
    	result,
    	confidence);
    }

    HAL_UART_Transmit(&huart3,
                      (uint8_t*)msg,
                      strlen(msg),
                      HAL_MAX_DELAY);

   // HAL_Delay(2000);
}
#endif

/* Multiple network support --------------------------------------------------*/

#include <string.h>
#include "ai_datatypes_defines.h"

static const ai_network_entry_t networks[AI_MNETWORK_NUMBER] = {
    {
        .name = (const char *)AI_NETWORK_MODEL_NAME,
        .config = AI_NETWORK_DATA_CONFIG,
        .ai_get_report = ai_network_get_report,
        .ai_create = ai_network_create,
        .ai_destroy = ai_network_destroy,
        .ai_get_error = ai_network_get_error,
        .ai_init = ai_network_init,
        .ai_run = ai_network_run,
        .ai_forward = ai_network_forward,
        .ai_data_params_get = ai_network_data_params_get,
        .activations = data_activations0
    },
};

struct network_instance {
     const ai_network_entry_t *entry;
     ai_handle handle;
     ai_network_params params;
};

/* Number of instance is aligned on the number of network */
AI_STATIC struct network_instance gnetworks[AI_MNETWORK_NUMBER] = {0};

AI_DECLARE_STATIC
ai_bool ai_mnetwork_is_valid(const char* name,
        const ai_network_entry_t *entry)
{
    if (name && (strlen(entry->name) == strlen(name)) &&
            (strncmp(entry->name, name, strlen(entry->name)) == 0))
        return true;
    return false;
}

AI_DECLARE_STATIC
struct network_instance *ai_mnetwork_handle(struct network_instance *inst)
{
    for (int i=0; i<AI_MNETWORK_NUMBER; i++) {
        if ((inst) && (&gnetworks[i] == inst))
            return inst;
        else if ((!inst) && (gnetworks[i].entry == NULL))
            return &gnetworks[i];
    }
    return NULL;
}

AI_DECLARE_STATIC
void ai_mnetwork_release_handle(struct network_instance *inst)
{
    for (int i=0; i<AI_MNETWORK_NUMBER; i++) {
        if ((inst) && (&gnetworks[i] == inst)) {
            gnetworks[i].entry = NULL;
            return;
        }
    }
}

AI_API_ENTRY
const char* ai_mnetwork_find(const char *name, ai_int idx)
{
    const ai_network_entry_t *entry;

    for (int i=0; i<AI_MNETWORK_NUMBER; i++) {
        entry = &networks[i];
        if (ai_mnetwork_is_valid(name, entry))
            return entry->name;
        else {
            if (!idx--)
                return entry->name;
        }
    }
    return NULL;
}

AI_API_ENTRY
ai_error ai_mnetwork_create(const char *name, ai_handle* network,
        const ai_buffer* network_config)
{
    const ai_network_entry_t *entry;
    const ai_network_entry_t *found = NULL;
    ai_error err;
    struct network_instance *inst = ai_mnetwork_handle(NULL);

    if (!inst) {
        err.type = AI_ERROR_ALLOCATION_FAILED;
        err.code = AI_ERROR_CODE_NETWORK;
        return err;
    }

    for (int i=0; i<AI_MNETWORK_NUMBER; i++) {
        entry = &networks[i];
        if (ai_mnetwork_is_valid(name, entry)) {
            found = entry;
            break;
        }
    }

    if (!found) {
        err.type = AI_ERROR_INVALID_PARAM;
        err.code = AI_ERROR_CODE_NETWORK;
        return err;
    }

    if (network_config == NULL)
        err = found->ai_create(network, found->config);
    else
        err = found->ai_create(network, network_config);
    if ((err.code == AI_ERROR_CODE_NONE) && (err.type == AI_ERROR_NONE)) {
        inst->entry = found;
        inst->handle = *network;
        *network = (ai_handle*)inst;
    }

    return err;
}

AI_API_ENTRY
ai_handle ai_mnetwork_destroy(ai_handle network)
{
    struct network_instance *inn;
    inn =  ai_mnetwork_handle((struct network_instance *)network);
    if (inn) {
        ai_handle hdl = inn->entry->ai_destroy(inn->handle);
        if (hdl != inn->handle) {
            ai_mnetwork_release_handle(inn);
            network = AI_HANDLE_NULL;
        }
    }
    return network;
}

AI_API_ENTRY
ai_bool ai_mnetwork_get_report(ai_handle network, ai_network_report* report)
{
    struct network_instance *inn;
    inn =  ai_mnetwork_handle((struct network_instance *)network);
    if (inn)
        return inn->entry->ai_get_report(inn->handle, report);
    else
        return false;
}

AI_API_ENTRY
ai_error ai_mnetwork_get_error(ai_handle network)
{
    struct network_instance *inn;
    ai_error err;
    err.type = AI_ERROR_INVALID_PARAM;
    err.code = AI_ERROR_CODE_NETWORK;

    inn =  ai_mnetwork_handle((struct network_instance *)network);
    if (inn)
        return inn->entry->ai_get_error(inn->handle);
    else
        return err;
}

AI_API_ENTRY
ai_bool ai_mnetwork_init(ai_handle network)
{
    struct network_instance *inn;
    ai_network_params par;

    inn =  ai_mnetwork_handle((struct network_instance *)network);
    if (inn) {
        inn->entry->ai_data_params_get(&par);
        for (int idx=0; idx < par.map_activations.size; idx++)
          AI_BUFFER_ARRAY_ITEM_SET_ADDRESS(&par.map_activations, idx, inn->entry->activations[idx]);
        return inn->entry->ai_init(inn->handle, &par);
    }
    else
        return false;
}

AI_API_ENTRY
ai_i32 ai_mnetwork_run(ai_handle network, const ai_buffer* input,
        ai_buffer* output)
{
    struct network_instance* inn;
    inn =  ai_mnetwork_handle((struct network_instance *)network);
    if (inn)
        return inn->entry->ai_run(inn->handle, input, output);
    else
        return 0;
}

AI_API_ENTRY
ai_i32 ai_mnetwork_forward(ai_handle network, const ai_buffer* input)
{
    struct network_instance *inn;
    inn =  ai_mnetwork_handle((struct network_instance *)network);
    if (inn)
        return inn->entry->ai_forward(inn->handle, input);
    else
        return 0;
}

AI_API_ENTRY
 int ai_mnetwork_get_private_handle(ai_handle network,
         ai_handle *phandle,
         ai_network_params *pparams)
 {
     struct network_instance* inn;
     inn =  ai_mnetwork_handle((struct network_instance *)network);
     if (inn && phandle && pparams) {
         *phandle = inn->handle;
         *pparams = inn->params;
         return 0;
     }
     else
         return -1;
 }

#ifdef __cplusplus
}
#endif
