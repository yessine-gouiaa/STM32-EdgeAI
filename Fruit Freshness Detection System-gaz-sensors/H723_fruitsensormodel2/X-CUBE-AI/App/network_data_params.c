/**
  ******************************************************************************
  * @file    network_data_params.c
  * @author  AST Embedded Analytics Research Platform
  * @date    Sun May 10 14:45:42 2026
  * @brief   AI Tool Automatic Code Generator for Embedded NN computing
  ******************************************************************************
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  ******************************************************************************
  */

#include "network_data_params.h"


/**  Activations Section  ****************************************************/
ai_handle g_network_activations_table[1 + 2] = {
  AI_HANDLE_PTR(AI_MAGIC_MARKER),
  AI_HANDLE_PTR(NULL),
  AI_HANDLE_PTR(AI_MAGIC_MARKER),
};




/**  Weights Section  ********************************************************/
AI_ALIGNED(32)
const ai_u64 s_network_weights_array_u64[40] = {
  0xd9b7ff2c818649d8U, 0xe5e5776f7f3581f4U, 0x81281ca1ad3b77fU, 0x14e80aa9c54881b2U,
  0xd920477f73587f4fU, 0x2e81c50281d476e1U, 0xb615cae47fd6b146U, 0x67f25d511e6ea7fU,
  0x812974d1bd7f49f7U, 0x81b1c3d95e9c44f8U, 0xfffffb48fffff6b8U, 0x75300000a8dU,
  0xb8100000031U, 0x14f400002018U, 0xffffffe6000003eaU, 0xbdf00000b29U,
  0x15d9fffffa22U, 0xfffffa02fffffe6cU, 0xf2ff1efb6e0c000dU, 0xac90b0f467fd246U,
  0xeaea1c3681f82700U, 0x20290dedcd9951edU, 0xf013043d8a041ae3U, 0x2149fafef08139e9U,
  0x1f5a962db77f19caU, 0x24345b0204c6abfaU, 0xc0505e97f0b0630U, 0x1ae61ded3955d32eU,
  0x170c20017f1c1614U, 0xfad425eb355dca17U, 0x1d9cb6b9608781fbU, 0x42c4a36120f22cb1U,
  0x112414e77f16041bU, 0x6e802072d76ed1dU, 0x3810000056bU, 0xfffffb5000000488U,
  0x41200000418U, 0x2d8fffffdb4U, 0x69ec6c7425c6b57fU, 0xffffff8fU,
};


ai_handle g_network_weights_table[1 + 2] = {
  AI_HANDLE_PTR(AI_MAGIC_MARKER),
  AI_HANDLE_PTR(s_network_weights_array_u64),
  AI_HANDLE_PTR(AI_MAGIC_MARKER),
};

