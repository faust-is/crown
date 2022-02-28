/************************************************************************
*
*	FILENAME		:	window.h
*
*	DESCRIPTION		:	Asymetric Hamming window for LPC analysis.
*
************************************************************************/
#ifndef __WINDOW_H
#define __WINDOW_H

#include "source.h"

const Word16 hamming_window[L_window] = {
  2621,  2623,  2628,  2636,  2647,  2662,  2679,  2700,  2724,  2752,
  2782,  2816,  2852,  2892,  2936,  2982,  3031,  3084,  3140,  3199,
  3260,  3325,  3393,  3465,  3539,  3616,  3696,  3779,  3865,  3954,
  4047,  4141,  4239,  4340,  4444,  4550,  4659,  4771,  4886,  5003,
  5123,  5246,  5372,  5500,  5631,  5764,  5900,  6038,  6179,  6323,
  6468,  6617,  6767,  6920,  7075,  7233,  7392,  7554,  7718,  7884,
  8052,  8223,  8395,  8569,  8746,  8924,  9104,  9286,  9470,  9655,
  9842, 10031, 10221, 10413, 10607, 10802, 10999, 11197, 11396, 11597,
 11799, 12002, 12207, 12413, 12619, 12827, 13036, 13246, 13457, 13669,
 13882, 14095, 14309, 14524, 14740, 14956, 15173, 15391, 15608, 15827,
 16046, 16265, 16484, 16704, 16924, 17144, 17364, 17584, 17804, 18024,
 18245, 18465, 18684, 18904, 19124, 19343, 19561, 19780, 19998, 20215,
 20432, 20648, 20864, 21079, 21293, 21506, 21719, 21931, 22142, 22352,
 22561, 22769, 22976, 23181, 23386, 23589, 23791, 23992, 24191, 24389,
 24586, 24781, 24975, 25167, 25357, 25546, 25733, 25919, 26102, 26284,
 26464, 26642, 26819, 26993, 27165, 27336, 27504, 27670, 27834, 27996,
 28156, 28313, 28468, 28621, 28772, 28920, 29066, 29209, 29350, 29488,
 29624, 29757, 29888, 30016, 30142, 30265, 30385, 30502, 30617, 30729,
 30838, 30945, 31048, 31149, 31247, 31342, 31434, 31523, 31609, 31692,
 31772, 31850, 31924, 31995, 32063, 32128, 32190, 32249, 32304, 32357,
 32406, 32453, 32496, 32536, 32573, 32606, 32637, 32664, 32688, 32709,
 32727, 32741, 32753, 32761, 32765, 32767, 32767, 32718, 32572, 32329,
 31991, 31561, 31041, 30434, 29744, 28977, 28136, 27227, 26257, 25231,
 24156, 23039, 21888, 20709, 19511, 18301, 17088, 15878, 14680, 13501,
 12350, 11233, 10158,  9132,  8162,  7253,  6412,  5645,  4955,  4348,
  3828,  3397,  3059,  2817,  2670,  2621 };

#endif