#ifndef HEADER_LDC_COEF
#define HEADER_LDC_COEF

#include <stdint.h>

uint16_t InputY[] = {
    406,  367,  818,  783,  366,  330,  782,  749,  329,  299,  748,  720,
    298,  272,  719,  695,  271,  250,  695,  676,  250,  235,  675,  662,
    234,  225,  661,  653,  225,  222,  653,  650,  222,  225,  650,  652,
    225,  234,  653,  661,  234,  249,  661,  675,  250,  270,  675,  694,
    271,  297,  695,  718,  298,  328,  719,  747,  329,  365,  748,  780,
    366,  405,  782,  817,  832,  796,  1257, 1225, 795,  763,  1224, 1196,
    762,  734,  1195, 1170, 733,  709,  1169, 1148, 709,  690,  1147, 1130,
    689,  676,  1130, 1117, 675,  667,  1117, 1110, 667,  664,  1109, 1107,
    664,  666,  1107, 1109, 667,  675,  1109, 1117, 675,  689,  1117, 1129,
    689,  708,  1130, 1146, 709,  732,  1147, 1168, 733,  761,  1169, 1194,
    762,  794,  1195, 1223, 795,  831,  1224, 1256, 1271, 1239, 1708, 1680,
    1238, 1210, 1679, 1655, 1209, 1184, 1654, 1632, 1183, 1162, 1632, 1613,
    1161, 1145, 1613, 1598, 1144, 1132, 1598, 1587, 1131, 1124, 1587, 1581,
    1124, 1121, 1580, 1578, 1121, 1124, 1578, 1580, 1124, 1131, 1580, 1587,
    1131, 1144, 1587, 1597, 1144, 1161, 1598, 1612, 1161, 1182, 1613, 1631,
    1183, 1208, 1632, 1653, 1209, 1237, 1654, 1679, 1238, 1270, 1679, 1707,
    1722, 1695, 2169, 2146, 1694, 1669, 2145, 2125, 1668, 1647, 2125, 2107,
    1646, 1628, 2106, 2091, 1628, 1613, 2091, 2079, 1613, 1602, 2078, 2070,
    1602, 1595, 2069, 2064, 1595, 1593, 2064, 2062, 1593, 1595, 2062, 2064,
    1595, 1602, 2064, 2069, 1602, 1612, 2069, 2078, 1613, 1627, 2078, 2090,
    1628, 1646, 2091, 2106, 1646, 1668, 2106, 2124, 1668, 1693, 2125, 2145,
    1694, 1721, 2145, 2168, 2183, 2161, 2639, 2621, 2160, 2140, 2621, 2605,
    2139, 2122, 2605, 2591, 2121, 2106, 2591, 2579, 2106, 2094, 2579, 2569,
    2094, 2085, 2569, 2562, 2085, 2079, 2562, 2558, 2079, 2077, 2558, 2557,
    2077, 2079, 2557, 2558, 2079, 2084, 2558, 2562, 2085, 2093, 2562, 2569,
    2094, 2105, 2569, 2578, 2106, 2121, 2579, 2590, 2121, 2139, 2591, 2604,
    2139, 2159, 2605, 2620, 2160, 2182, 2621, 2638, 2653, 2636, 3115, 3103,
    2636, 2620, 3103, 3092, 2620, 2606, 3092, 3083, 2606, 2594, 3083, 3075,
    2594, 2585, 3074, 3068, 2585, 2578, 3068, 3063, 2578, 2574, 3063, 3061,
    2574, 2572, 3060, 3059, 2572, 2574, 3059, 3060, 2574, 2578, 3060, 3063,
    2578, 2584, 3063, 3068, 2585, 2594, 3068, 3074, 2594, 2605, 3074, 3082,
    2606, 2619, 3083, 3092, 2620, 2635, 3092, 3103, 2636, 2653, 3103, 3115,
    3130, 3119, 3597, 3591, 3118, 3108, 3590, 3585, 3107, 3098, 3585, 3580,
    3098, 3090, 3580, 3576, 3090, 3084, 3576, 3572, 3084, 3079, 3572, 3570,
    3079, 3076, 3570, 3569, 3076, 3075, 3569, 3568, 3075, 3076, 3568, 3568,
    3076, 3079, 3569, 3570, 3079, 3084, 3570, 3572, 3084, 3090, 3572, 3576,
    3090, 3098, 3576, 3580, 3098, 3107, 3580, 3585, 3107, 3118, 3585, 3590,
    3118, 3130, 3590, 3596, 3612, 3606, 4080, 4080, 3606, 3600, 4080, 4080,
    3600, 3596, 4080, 4080, 3595, 3592, 4080, 4080, 3591, 3588, 4080, 4080,
    3588, 3586, 4080, 4080, 3586, 3584, 4080, 4080, 3584, 3584, 4080, 4080,
    3584, 3584, 4080, 4080, 3584, 3586, 4080, 4080, 3586, 3588, 4080, 4080,
    3588, 3591, 4080, 4080, 3591, 3595, 4080, 4080, 3595, 3600, 4080, 4080,
    3600, 3605, 4080, 4080, 3606, 3611, 4080, 4080, 4096, 4096, 4564, 4570,
    4096, 4096, 4570, 4575, 4096, 4096, 4575, 4580, 4096, 4096, 4580, 4584,
    4096, 4096, 4584, 4587, 4096, 4096, 4587, 4589, 4096, 4096, 4589, 4591,
    4096, 4096, 4591, 4591, 4096, 4096, 4591, 4591, 4096, 4096, 4591, 4589,
    4096, 4096, 4589, 4587, 4096, 4096, 4587, 4584, 4096, 4096, 4584, 4580,
    4096, 4096, 4580, 4575, 4096, 4096, 4575, 4570, 4096, 4096, 4570, 4564,
    4579, 4585, 5046, 5057, 4585, 4591, 5058, 5068, 4591, 4595, 5068, 5077,
    4596, 4599, 5077, 5085, 4600, 4603, 5085, 5091, 4603, 4605, 5092, 5096,
    4605, 4607, 5096, 5099, 4607, 4607, 5099, 5100, 4607, 4607, 5100, 5099,
    4607, 4605, 5099, 5096, 4605, 4603, 5096, 5092, 4603, 4600, 5092, 5086,
    4600, 4596, 5085, 5078, 4596, 4591, 5077, 5068, 4591, 4586, 5068, 5058,
    4585, 4580, 5058, 5046, 5061, 5072, 5523, 5540, 5073, 5083, 5540, 5556,
    5084, 5093, 5556, 5569, 5093, 5101, 5570, 5581, 5101, 5107, 5582, 5591,
    5107, 5112, 5591, 5597, 5112, 5115, 5598, 5602, 5115, 5116, 5602, 5603,
    5116, 5115, 5603, 5602, 5115, 5112, 5602, 5598, 5112, 5107, 5598, 5591,
    5107, 5101, 5591, 5582, 5101, 5093, 5582, 5570, 5093, 5084, 5570, 5557,
    5084, 5073, 5556, 5541, 5073, 5061, 5540, 5523, 5538, 5555, 5993, 6015,
    5555, 5571, 6016, 6036, 5571, 5585, 6037, 6054, 5585, 5597, 6055, 6070,
    5597, 5606, 6070, 6082, 5606, 5613, 6082, 6091, 5613, 5617, 6091, 6096,
    5617, 5619, 6096, 6098, 5619, 5617, 6098, 6097, 5617, 5613, 6096, 6091,
    5613, 5607, 6091, 6083, 5606, 5597, 6082, 6070, 5597, 5586, 6070, 6055,
    5585, 5572, 6055, 6037, 5571, 5556, 6037, 6017, 5555, 5538, 6016, 5994,
    6008, 6030, 6455, 6482, 6031, 6051, 6483, 6507, 6052, 6069, 6508, 6529,
    6070, 6085, 6530, 6548, 6085, 6097, 6549, 6563, 6097, 6106, 6563, 6574,
    6106, 6112, 6574, 6581, 6112, 6114, 6581, 6583, 6114, 6112, 6583, 6581,
    6112, 6107, 6581, 6574, 6106, 6098, 6574, 6564, 6097, 6086, 6563, 6549,
    6085, 6070, 6549, 6531, 6070, 6052, 6530, 6509, 6052, 6032, 6508, 6484,
    6031, 6009, 6483, 6456, 6469, 6496, 6906, 6938, 6497, 6522, 6939, 6967,
    6523, 6544, 6968, 6993, 6545, 6563, 6994, 7015, 6563, 6578, 7015, 7032,
    6578, 6589, 7032, 7045, 6589, 6596, 7045, 7052, 6596, 6598, 7053, 7055,
    6598, 6596, 7055, 7053, 6596, 6589, 7053, 7045, 6589, 6579, 7045, 7033,
    6578, 6564, 7032, 7016, 6563, 6545, 7015, 6994, 6545, 6523, 6994, 6969,
    6523, 6498, 6968, 6940, 6497, 6470, 6939, 6907, 6920, 6952, 7345, 7381,
    6953, 6981, 7382, 7414, 6982, 7007, 7415, 7443, 7008, 7029, 7444, 7468,
    7030, 7046, 7468, 7487, 7047, 7059, 7488, 7501, 7060, 7067, 7502, 7510,
    7067, 7070, 7510, 7513, 7070, 7067, 7513, 7511, 7067, 7060, 7510, 7502,
    7060, 7047, 7502, 7488, 7047, 7030, 7488, 7469, 7030, 7009, 7468, 7445,
    7008, 6983, 7444, 7416, 6982, 6954, 7415, 7383, 6953, 6921, 7382, 7347,
    7359, 7395, 7772, 7811, 7396, 7428, 7812, 7847, 7429, 7457, 7848, 7879,
    7458, 7482, 7880, 7906, 7482, 7501, 7907, 7927, 7502, 7515, 7928, 7943,
    7516, 7524, 7943, 7952, 7524, 7527, 7953, 7956, 7527, 7525, 7956, 7953,
    7524, 7516, 7953, 7944, 7516, 7502, 7943, 7928, 7502, 7483, 7928, 7907,
    7482, 7459, 7907, 7881, 7458, 7430, 7880, 7849, 7429, 7397, 7848, 7813,
    7396, 7360, 7812, 7773};
uint16_t InputX[] = {
    406,  818,  367,  783,  832,  1257, 796,  1225, 1271, 1708, 1239, 1680,
    1722, 2169, 1695, 2146, 2183, 2639, 2161, 2621, 2653, 3115, 2636, 3103,
    3130, 3597, 3119, 3591, 3612, 4080, 3606, 4080, 4096, 4564, 4096, 4570,
    4579, 5046, 4585, 5057, 5061, 5523, 5072, 5540, 5538, 5993, 5555, 6015,
    6008, 6455, 6030, 6482, 6469, 6906, 6496, 6938, 6920, 7345, 6952, 7381,
    7359, 7772, 7395, 7811, 366,  782,  330,  749,  795,  1224, 763,  1196,
    1238, 1679, 1210, 1655, 1694, 2145, 1669, 2125, 2160, 2621, 2140, 2605,
    2636, 3103, 2620, 3092, 3118, 3590, 3108, 3585, 3606, 4080, 3600, 4080,
    4096, 4570, 4096, 4575, 4585, 5058, 4591, 5068, 5073, 5540, 5083, 5556,
    5555, 6016, 5571, 6036, 6031, 6483, 6051, 6507, 6497, 6939, 6522, 6967,
    6953, 7382, 6981, 7414, 7396, 7812, 7428, 7847, 329,  748,  299,  720,
    762,  1195, 734,  1170, 1209, 1654, 1184, 1632, 1668, 2125, 1647, 2107,
    2139, 2605, 2122, 2591, 2620, 3092, 2606, 3083, 3107, 3585, 3098, 3580,
    3600, 4080, 3596, 4080, 4096, 4575, 4096, 4580, 4591, 5068, 4595, 5077,
    5084, 5556, 5093, 5569, 5571, 6037, 5585, 6054, 6052, 6508, 6069, 6529,
    6523, 6968, 6544, 6993, 6982, 7415, 7007, 7443, 7429, 7848, 7457, 7879,
    298,  719,  272,  695,  733,  1169, 709,  1148, 1183, 1632, 1162, 1613,
    1646, 2106, 1628, 2091, 2121, 2591, 2106, 2579, 2606, 3083, 2594, 3075,
    3098, 3580, 3090, 3576, 3595, 4080, 3592, 4080, 4096, 4580, 4096, 4584,
    4596, 5077, 4599, 5085, 5093, 5570, 5101, 5581, 5585, 6055, 5597, 6070,
    6070, 6530, 6085, 6548, 6545, 6994, 6563, 7015, 7008, 7444, 7029, 7468,
    7458, 7880, 7482, 7906, 271,  695,  250,  676,  709,  1147, 690,  1130,
    1161, 1613, 1145, 1598, 1628, 2091, 1613, 2079, 2106, 2579, 2094, 2569,
    2594, 3074, 2585, 3068, 3090, 3576, 3084, 3572, 3591, 4080, 3588, 4080,
    4096, 4584, 4096, 4587, 4600, 5085, 4603, 5091, 5101, 5582, 5107, 5591,
    5597, 6070, 5606, 6082, 6085, 6549, 6097, 6563, 6563, 7015, 6578, 7032,
    7030, 7468, 7046, 7487, 7482, 7907, 7501, 7927, 250,  675,  235,  662,
    689,  1130, 676,  1117, 1144, 1598, 1132, 1587, 1613, 2078, 1602, 2070,
    2094, 2569, 2085, 2562, 2585, 3068, 2578, 3063, 3084, 3572, 3079, 3570,
    3588, 4080, 3586, 4080, 4096, 4587, 4096, 4589, 4603, 5092, 4605, 5096,
    5107, 5591, 5112, 5597, 5606, 6082, 5613, 6091, 6097, 6563, 6106, 6574,
    6578, 7032, 6589, 7045, 7047, 7488, 7059, 7501, 7502, 7928, 7515, 7943,
    234,  661,  225,  653,  675,  1117, 667,  1110, 1131, 1587, 1124, 1581,
    1602, 2069, 1595, 2064, 2085, 2562, 2079, 2558, 2578, 3063, 2574, 3061,
    3079, 3570, 3076, 3569, 3586, 4080, 3584, 4080, 4096, 4589, 4096, 4591,
    4605, 5096, 4607, 5099, 5112, 5598, 5115, 5602, 5613, 6091, 5617, 6096,
    6106, 6574, 6112, 6581, 6589, 7045, 6596, 7052, 7060, 7502, 7067, 7510,
    7516, 7943, 7524, 7952, 225,  653,  222,  650,  667,  1109, 664,  1107,
    1124, 1580, 1121, 1578, 1595, 2064, 1593, 2062, 2079, 2558, 2077, 2557,
    2574, 3060, 2572, 3059, 3076, 3569, 3075, 3568, 3584, 4080, 3584, 4080,
    4096, 4591, 4096, 4591, 4607, 5099, 4607, 5100, 5115, 5602, 5116, 5603,
    5617, 6096, 5619, 6098, 6112, 6581, 6114, 6583, 6596, 7053, 6598, 7055,
    7067, 7510, 7070, 7513, 7524, 7953, 7527, 7956, 222,  650,  225,  652,
    664,  1107, 666,  1109, 1121, 1578, 1124, 1580, 1593, 2062, 1595, 2064,
    2077, 2557, 2079, 2558, 2572, 3059, 2574, 3060, 3075, 3568, 3076, 3568,
    3584, 4080, 3584, 4080, 4096, 4591, 4096, 4591, 4607, 5100, 4607, 5099,
    5116, 5603, 5115, 5602, 5619, 6098, 5617, 6097, 6114, 6583, 6112, 6581,
    6598, 7055, 6596, 7053, 7070, 7513, 7067, 7511, 7527, 7956, 7525, 7953,
    225,  653,  234,  661,  667,  1109, 675,  1117, 1124, 1580, 1131, 1587,
    1595, 2064, 1602, 2069, 2079, 2558, 2084, 2562, 2574, 3060, 2578, 3063,
    3076, 3569, 3079, 3570, 3584, 4080, 3586, 4080, 4096, 4591, 4096, 4589,
    4607, 5099, 4605, 5096, 5115, 5602, 5112, 5598, 5617, 6096, 5613, 6091,
    6112, 6581, 6107, 6574, 6596, 7053, 6589, 7045, 7067, 7510, 7060, 7502,
    7524, 7953, 7516, 7944, 234,  661,  249,  675,  675,  1117, 689,  1129,
    1131, 1587, 1144, 1597, 1602, 2069, 1612, 2078, 2085, 2562, 2093, 2569,
    2578, 3063, 2584, 3068, 3079, 3570, 3084, 3572, 3586, 4080, 3588, 4080,
    4096, 4589, 4096, 4587, 4605, 5096, 4603, 5092, 5112, 5598, 5107, 5591,
    5613, 6091, 5607, 6083, 6106, 6574, 6098, 6564, 6589, 7045, 6579, 7033,
    7060, 7502, 7047, 7488, 7516, 7943, 7502, 7928, 250,  675,  270,  694,
    689,  1130, 708,  1146, 1144, 1598, 1161, 1612, 1613, 2078, 1627, 2090,
    2094, 2569, 2105, 2578, 2585, 3068, 2594, 3074, 3084, 3572, 3090, 3576,
    3588, 4080, 3591, 4080, 4096, 4587, 4096, 4584, 4603, 5092, 4600, 5086,
    5107, 5591, 5101, 5582, 5606, 6082, 5597, 6070, 6097, 6563, 6086, 6549,
    6578, 7032, 6564, 7016, 7047, 7488, 7030, 7469, 7502, 7928, 7483, 7907,
    271,  695,  297,  718,  709,  1147, 732,  1168, 1161, 1613, 1182, 1631,
    1628, 2091, 1646, 2106, 2106, 2579, 2121, 2590, 2594, 3074, 2605, 3082,
    3090, 3576, 3098, 3580, 3591, 4080, 3595, 4080, 4096, 4584, 4096, 4580,
    4600, 5085, 4596, 5078, 5101, 5582, 5093, 5570, 5597, 6070, 5586, 6055,
    6085, 6549, 6070, 6531, 6563, 7015, 6545, 6994, 7030, 7468, 7009, 7445,
    7482, 7907, 7459, 7881, 298,  719,  328,  747,  733,  1169, 761,  1194,
    1183, 1632, 1208, 1653, 1646, 2106, 1668, 2124, 2121, 2591, 2139, 2604,
    2606, 3083, 2619, 3092, 3098, 3580, 3107, 3585, 3595, 4080, 3600, 4080,
    4096, 4580, 4096, 4575, 4596, 5077, 4591, 5068, 5093, 5570, 5084, 5557,
    5585, 6055, 5572, 6037, 6070, 6530, 6052, 6509, 6545, 6994, 6523, 6969,
    7008, 7444, 6983, 7416, 7458, 7880, 7430, 7849, 329,  748,  365,  780,
    762,  1195, 794,  1223, 1209, 1654, 1237, 1679, 1668, 2125, 1693, 2145,
    2139, 2605, 2159, 2620, 2620, 3092, 2635, 3103, 3107, 3585, 3118, 3590,
    3600, 4080, 3605, 4080, 4096, 4575, 4096, 4570, 4591, 5068, 4586, 5058,
    5084, 5556, 5073, 5541, 5571, 6037, 5556, 6017, 6052, 6508, 6032, 6484,
    6523, 6968, 6498, 6940, 6982, 7415, 6954, 7383, 7429, 7848, 7397, 7813,
    366,  782,  405,  817,  795,  1224, 831,  1256, 1238, 1679, 1270, 1707,
    1694, 2145, 1721, 2168, 2160, 2621, 2182, 2638, 2636, 3103, 2653, 3115,
    3118, 3590, 3130, 3596, 3606, 4080, 3611, 4080, 4096, 4570, 4096, 4564,
    4585, 5058, 4580, 5046, 5073, 5540, 5061, 5523, 5555, 6016, 5538, 5994,
    6031, 6483, 6009, 6456, 6497, 6939, 6470, 6907, 6953, 7382, 6921, 7347,
    7396, 7812, 7360, 7773};

uint16_t InputBB[] = {
    /*X , Y , Width, Height*/
    22,  22,  32, 32, 48,  20,  34, 32, 76,  18,  34, 32, 104, 16,  34, 32,
    134, 14,  34, 32, 164, 14,  34, 32, 194, 14,  34, 30, 224, 12,  34, 32,
    256, 12,  32, 32, 286, 14,  34, 30, 316, 14,  34, 32, 346, 14,  32, 32,
    374, 16,  34, 32, 404, 18,  32, 32, 432, 20,  32, 32, 458, 22,  34, 32,
    20,  48,  32, 34, 46,  46,  34, 34, 74,  44,  34, 34, 104, 44,  34, 32,
    132, 42,  34, 32, 162, 42,  34, 32, 194, 40,  34, 32, 224, 40,  34, 32,
    256, 40,  32, 32, 286, 40,  34, 32, 316, 42,  34, 32, 346, 42,  34, 32,
    376, 44,  34, 32, 406, 44,  32, 34, 434, 46,  32, 34, 462, 48,  32, 34,
    18,  76,  32, 34, 44,  74,  34, 34, 74,  74,  32, 32, 102, 72,  34, 34,
    132, 70,  34, 34, 162, 70,  34, 32, 192, 70,  36, 32, 224, 70,  34, 32,
    256, 70,  34, 32, 286, 70,  34, 32, 316, 70,  36, 32, 348, 70,  34, 34,
    378, 72,  34, 32, 406, 72,  34, 34, 436, 74,  32, 34, 464, 76,  32, 34,
    16,  104, 32, 34, 44,  104, 32, 34, 72,  102, 34, 34, 100, 100, 34, 34,
    130, 100, 34, 34, 162, 100, 34, 32, 192, 98,  34, 34, 224, 98,  34, 34,
    256, 98,  34, 34, 286, 98,  34, 34, 318, 100, 34, 32, 348, 100, 34, 34,
    378, 100, 34, 34, 408, 102, 34, 34, 438, 104, 32, 34, 466, 104, 32, 34,
    14,  134, 32, 34, 42,  132, 32, 34, 70,  132, 34, 34, 100, 130, 34, 34,
    130, 130, 34, 34, 160, 130, 36, 34, 192, 128, 34, 36, 224, 128, 34, 34,
    256, 128, 34, 34, 286, 128, 36, 36, 318, 130, 34, 34, 348, 130, 36, 34,
    380, 130, 34, 34, 410, 132, 32, 34, 438, 132, 32, 34, 466, 134, 32, 34,
    14,  164, 32, 34, 42,  162, 32, 34, 70,  162, 32, 34, 100, 162, 32, 34,
    130, 160, 34, 36, 160, 160, 34, 34, 192, 160, 34, 34, 224, 160, 34, 34,
    256, 160, 34, 34, 286, 160, 36, 34, 318, 160, 34, 34, 350, 160, 34, 36,
    380, 162, 34, 34, 410, 162, 34, 34, 440, 162, 32, 34, 468, 164, 32, 34,
    14,  194, 30, 34, 40,  194, 32, 34, 70,  192, 32, 36, 98,  192, 34, 34,
    128, 192, 36, 34, 160, 192, 34, 34, 192, 192, 34, 34, 224, 192, 34, 34,
    256, 192, 34, 34, 286, 192, 36, 34, 318, 192, 36, 34, 350, 192, 34, 34,
    380, 192, 34, 34, 410, 192, 34, 36, 440, 194, 32, 34, 468, 194, 32, 34,
    12,  224, 32, 34, 40,  224, 32, 34, 70,  224, 32, 34, 98,  224, 34, 34,
    128, 224, 34, 34, 160, 224, 34, 34, 192, 224, 34, 34, 224, 224, 34, 34,
    256, 224, 34, 34, 286, 224, 36, 34, 318, 224, 36, 34, 350, 224, 34, 34,
    382, 224, 32, 34, 412, 224, 32, 34, 440, 224, 32, 34, 470, 224, 30, 34,
    12,  256, 32, 32, 40,  256, 32, 32, 70,  256, 32, 34, 98,  256, 34, 34,
    128, 256, 34, 34, 160, 256, 34, 34, 192, 256, 34, 34, 224, 256, 34, 34,
    256, 256, 34, 34, 286, 256, 36, 34, 318, 256, 36, 34, 350, 256, 34, 34,
    382, 256, 32, 34, 412, 256, 32, 34, 440, 256, 32, 32, 470, 256, 30, 32,
    14,  286, 30, 34, 40,  286, 32, 34, 70,  286, 32, 34, 98,  286, 34, 34,
    128, 286, 36, 36, 160, 286, 34, 36, 192, 286, 34, 36, 224, 286, 34, 36,
    256, 286, 34, 36, 286, 286, 36, 36, 318, 286, 36, 36, 350, 286, 34, 36,
    380, 286, 34, 34, 410, 286, 34, 34, 440, 286, 32, 34, 468, 286, 32, 34,
    14,  316, 32, 34, 42,  316, 32, 34, 70,  316, 32, 36, 100, 318, 32, 34,
    130, 318, 34, 34, 160, 318, 34, 34, 192, 318, 34, 36, 224, 318, 34, 36,
    256, 318, 34, 36, 286, 318, 36, 36, 318, 318, 34, 34, 350, 318, 34, 34,
    380, 318, 34, 34, 410, 316, 34, 36, 440, 316, 32, 34, 468, 316, 32, 34,
    14,  346, 32, 32, 42,  346, 32, 34, 70,  348, 34, 34, 100, 348, 34, 34,
    130, 348, 34, 36, 160, 350, 36, 34, 192, 350, 34, 34, 224, 350, 34, 34,
    256, 350, 34, 34, 286, 350, 36, 34, 318, 350, 34, 34, 348, 348, 36, 36,
    380, 348, 34, 34, 410, 348, 32, 34, 438, 346, 34, 34, 466, 346, 32, 34,
    16,  374, 32, 34, 44,  376, 32, 34, 72,  378, 32, 34, 100, 378, 34, 34,
    130, 380, 34, 34, 162, 380, 34, 34, 192, 380, 34, 34, 224, 382, 34, 32,
    256, 382, 34, 32, 286, 380, 34, 34, 318, 380, 34, 34, 348, 380, 34, 34,
    378, 378, 34, 34, 408, 378, 34, 34, 438, 376, 32, 34, 466, 374, 32, 34,
    18,  404, 32, 32, 44,  406, 34, 32, 72,  406, 34, 34, 102, 408, 34, 34,
    132, 410, 34, 32, 162, 410, 34, 34, 192, 410, 36, 34, 224, 412, 34, 32,
    256, 412, 34, 32, 286, 410, 34, 34, 316, 410, 36, 34, 348, 410, 34, 32,
    378, 408, 34, 34, 406, 406, 34, 34, 436, 406, 32, 32, 464, 404, 32, 32,
    20,  432, 32, 32, 46,  434, 34, 32, 74,  436, 34, 32, 104, 438, 34, 32,
    132, 438, 34, 32, 162, 440, 34, 32, 194, 440, 34, 32, 224, 440, 34, 32,
    256, 440, 32, 32, 286, 440, 34, 32, 316, 440, 34, 32, 346, 438, 34, 34,
    376, 438, 34, 32, 406, 436, 32, 32, 434, 434, 32, 32, 462, 432, 32, 32,
    22,  458, 32, 34, 48,  462, 34, 32, 76,  464, 34, 32, 104, 466, 34, 32,
    134, 466, 34, 32, 164, 468, 34, 32, 194, 468, 34, 32, 224, 470, 34, 30,
    256, 470, 32, 30, 286, 468, 34, 32, 316, 468, 34, 32, 346, 466, 34, 32,
    374, 466, 34, 32, 404, 464, 32, 32, 432, 462, 32, 32, 460, 460, 32, 32};
#endif
