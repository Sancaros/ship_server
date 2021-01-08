#pragma once

//用于每次更新代码与之前代码做比对

//为0时调试关闭， 为1时调试打开

#if 0

//开启更新代码debug
#define DEBUG //更新的DEBUG代码 用于debug 12.23

#else

//关闭更新代码debug
#define NO_DEBUG //更新的DEBUG代码 用于debug 12.23
#endif

#if 0

//开启更新代码debug
#define UPDATE_DEBUG  //更新的DEBUG代码 用于debug 12.23

#else

//关闭更新代码debug
#define NOUPDATE_DEBUG //更新的DEBUG代码 用于debug 12.23
#endif


