/**********************************************************************************
*   Created 2018 by Johny Pham
*   tuaanpv@gmail.com
*   Youtube/Tiktok/Instagram: tuaanpv
***********************************************************************************/

#ifndef __COLOR_TEXT_H__
#define __COLOR_TEXT_H__

#include <dlib/pixel.h>

#include <stdio.h>
#include <string>
#include <unistd.h>
#include <ctime>
#include <thread>
#include <mutex>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <sys/types.h>
#include <dirent.h>

using namespace std;

#define CRESET   "\033[0m"
#define CBLACK   "\033[30m"      /* Black */
#define CRED     "\033[31m"      /* Red */
#define CGREEN   "\033[32m"      /* Green */
#define CYELLOW  "\033[33m"      /* Yellow */
#define CBLUE    "\033[34m"      /* Blue */
#define CMAGENTA "\033[35m"      /* Magenta */
#define CCYAN    "\033[36m"      /* Cyan */
#define CWHITE   "\033[37m"      /* White */
#define CBOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define CBOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define CBOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define CBOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define CBOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define CBOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define CBOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define CBOLDWHITE   "\033[1m\033[37m"      /* Bold White */ 
#define CCLEAR "\033[2J"  // clear screen escape code 


//-----------------------------------------------------------
// reference to page: https://www.rapidtables.com/web/color/RGB_Color.html
//
// typedef    dlib::rgb_pixel Colour;

#define    color_maroon    dlib::rgb_pixel(128,0,0)
#define    color_dark_red    dlib::rgb_pixel(139,0,0)
#define    color_brown    dlib::rgb_pixel(165,42,42)
#define    color_firebrick    dlib::rgb_pixel(178,34,34)
#define    color_crimson    dlib::rgb_pixel(220,20,60)
#define    color_red    dlib::rgb_pixel(255,0,0)
#define    color_tomato    dlib::rgb_pixel(255,99,71)
#define    color_coral    dlib::rgb_pixel(255,127,80)
#define    color_indian_red    dlib::rgb_pixel(205,92,92)
#define    color_light_coral    dlib::rgb_pixel(240,128,128)
#define    color_dark_salmon    dlib::rgb_pixel(233,150,122)
#define    color_salmon    dlib::rgb_pixel(250,128,114)
#define    color_light_salmon    dlib::rgb_pixel(255,160,122)
#define    color_orange_red    dlib::rgb_pixel(255,69,0)
#define    color_dark_orange    dlib::rgb_pixel(255,140,0)
#define    color_orange    dlib::rgb_pixel(255,165,0)
#define    color_gold    dlib::rgb_pixel(255,215,0)
#define    color_dark_golden_rod    dlib::rgb_pixel(184,134,11)
#define    color_golden_rod    dlib::rgb_pixel(218,165,32)
#define    color_pale_golden_rod    dlib::rgb_pixel(238,232,170)
#define    color_dark_khaki    dlib::rgb_pixel(189,183,107)
#define    color_khaki    dlib::rgb_pixel(240,230,140)
#define    color_olive    dlib::rgb_pixel(128,128,0)
#define    color_yellow    dlib::rgb_pixel(255,255,0)
#define    color_yellow_green    dlib::rgb_pixel(154,205,50)
#define    color_dark_olive_green    dlib::rgb_pixel(85,107,47)
#define    color_olive_drab    dlib::rgb_pixel(107,142,35)
#define    color_lawn_green    dlib::rgb_pixel(124,252,0)
#define    color_chart_reuse    dlib::rgb_pixel(127,255,0)
#define    color_green_yellow    dlib::rgb_pixel(173,255,47)
#define    color_dark_green    dlib::rgb_pixel(0,100,0)
#define    color_green    dlib::rgb_pixel(0,128,0)
#define    color_forest_green    dlib::rgb_pixel(34,139,34)
#define    color_lime    dlib::rgb_pixel(0,255,0)
#define    color_lime_green    dlib::rgb_pixel(50,205,50)
#define    color_light_green    dlib::rgb_pixel(144,238,144)
#define    color_pale_green    dlib::rgb_pixel(152,251,152)
#define    color_dark_sea_green    dlib::rgb_pixel(143,188,143)
#define    color_medium_spring_green    dlib::rgb_pixel(0,250,154)
#define    color_spring_green    dlib::rgb_pixel(0,255,127)
#define    color_sea_green    dlib::rgb_pixel(46,139,87)
#define    color_medium_aqua_marine    dlib::rgb_pixel(102,205,170)
#define    color_medium_sea_green    dlib::rgb_pixel(60,179,113)
#define    color_light_sea_green    dlib::rgb_pixel(32,178,170)
#define    color_dark_slate_gray    dlib::rgb_pixel(47,79,79)
#define    color_teal    dlib::rgb_pixel(0,128,128)
#define    color_dark_cyan    dlib::rgb_pixel(0,139,139)
#define    color_aqua    dlib::rgb_pixel(0,255,255)
#define    color_cyan    dlib::rgb_pixel(0,255,255)
#define    color_light_cyan    dlib::rgb_pixel(224,255,255)
#define    color_dark_turquoise    dlib::rgb_pixel(0,206,209)
#define    color_turquoise    dlib::rgb_pixel(64,224,208)
#define    color_medium_turquoise    dlib::rgb_pixel(72,209,204)
#define    color_pale_turquoise    dlib::rgb_pixel(175,238,238)
#define    color_aqua_marine    dlib::rgb_pixel(127,255,212)
#define    color_powder_blue    dlib::rgb_pixel(176,224,230)
#define    color_cadet_blue    dlib::rgb_pixel(95,158,160)
#define    color_steel_blue    dlib::rgb_pixel(70,130,180)
#define    color_corn_flower_blue    dlib::rgb_pixel(100,149,237)
#define    color_deep_sky_blue    dlib::rgb_pixel(0,191,255)
#define    color_dodger_blue    dlib::rgb_pixel(30,144,255)
#define    color_light_blue    dlib::rgb_pixel(173,216,230)
#define    color_sky_blue    dlib::rgb_pixel(135,206,235)
#define    color_light_sky_blue    dlib::rgb_pixel(135,206,250)
#define    color_midnight_blue    dlib::rgb_pixel(25,25,112)
#define    color_navy    dlib::rgb_pixel(0,0,128)
#define    color_dark_blue    dlib::rgb_pixel(0,0,139)
#define    color_medium_blue    dlib::rgb_pixel(0,0,205)
#define    color_blue    dlib::rgb_pixel(0,0,255)
#define    color_royal_blue    dlib::rgb_pixel(65,105,225)
#define    color_blue_violet    dlib::rgb_pixel(138,43,226)
#define    color_indigo    dlib::rgb_pixel(75,0,130)
#define    color_dark_slate_blue    dlib::rgb_pixel(72,61,139)
#define    color_slate_blue    dlib::rgb_pixel(106,90,205)
#define    color_medium_slate_blue    dlib::rgb_pixel(123,104,238)
#define    color_medium_purple    dlib::rgb_pixel(147,112,219)
#define    color_dark_magenta    dlib::rgb_pixel(139,0,139)
#define    color_dark_violet    dlib::rgb_pixel(148,0,211)
#define    color_dark_orchid    dlib::rgb_pixel(153,50,204)
#define    color_medium_orchid    dlib::rgb_pixel(186,85,211)
#define    color_purple    dlib::rgb_pixel(128,0,128)
#define    color_thistle    dlib::rgb_pixel(216,191,216)
#define    color_plum    dlib::rgb_pixel(221,160,221)
#define    color_violet    dlib::rgb_pixel(238,130,238)
#define    color_magenta_fuchsia    dlib::rgb_pixel(255,0,255)
#define    color_orchid    dlib::rgb_pixel(218,112,214)
#define    color_medium_violet_red    dlib::rgb_pixel(199,21,133)
#define    color_pale_violet_red    dlib::rgb_pixel(219,112,147)
#define    color_deep_pink    dlib::rgb_pixel(255,20,147)
#define    color_hot_pink    dlib::rgb_pixel(255,105,180)
#define    color_light_pink    dlib::rgb_pixel(255,182,193)
#define    color_pink    dlib::rgb_pixel(255,192,203)
#define    color_antique_white    dlib::rgb_pixel(250,235,215)
#define    color_beige    dlib::rgb_pixel(245,245,220)
#define    color_bisque    dlib::rgb_pixel(255,228,196)
#define    color_blanched_almond    dlib::rgb_pixel(255,235,205)
#define    color_wheat    dlib::rgb_pixel(245,222,179)
#define    color_corn_silk    dlib::rgb_pixel(255,248,220)
#define    color_lemon_chiffon    dlib::rgb_pixel(255,250,205)
#define    color_light_golden_rod_yellow    dlib::rgb_pixel(250,250,210)
#define    color_light_yellow    dlib::rgb_pixel(255,255,224)
#define    color_saddle_brown    dlib::rgb_pixel(139,69,19)
#define    color_sienna    dlib::rgb_pixel(160,82,45)
#define    color_chocolate    dlib::rgb_pixel(210,105,30)
#define    color_peru    dlib::rgb_pixel(205,133,63)
#define    color_sandy_brown    dlib::rgb_pixel(244,164,96)
#define    color_burly_wood    dlib::rgb_pixel(222,184,135)
#define    color_tan    dlib::rgb_pixel(210,180,140)
#define    color_rosy_brown    dlib::rgb_pixel(188,143,143)
#define    color_moccasin    dlib::rgb_pixel(255,228,181)
#define    color_navajo_white    dlib::rgb_pixel(255,222,173)
#define    color_peach_puff    dlib::rgb_pixel(255,218,185)
#define    color_misty_rose    dlib::rgb_pixel(255,228,225)
#define    color_lavender_blush    dlib::rgb_pixel(255,240,245)
#define    color_linen    dlib::rgb_pixel(250,240,230)
#define    color_old_lace    dlib::rgb_pixel(253,245,230)
#define    color_papaya_whip    dlib::rgb_pixel(255,239,213)
#define    color_sea_shell    dlib::rgb_pixel(255,245,238)
#define    color_mint_cream    dlib::rgb_pixel(245,255,250)
#define    color_slate_gray    dlib::rgb_pixel(112,128,144)
#define    color_light_slate_gray    dlib::rgb_pixel(119,136,153)
#define    color_light_steel_blue    dlib::rgb_pixel(176,196,222)
#define    color_lavender    dlib::rgb_pixel(230,230,250)
#define    color_floral_white    dlib::rgb_pixel(255,250,240)
#define    color_alice_blue    dlib::rgb_pixel(240,248,255)
#define    color_ghost_white    dlib::rgb_pixel(248,248,255)
#define    color_honeydew    dlib::rgb_pixel(240,255,240)
#define    color_ivory    dlib::rgb_pixel(255,255,240)
#define    color_azure    dlib::rgb_pixel(240,255,255)
#define    color_snow    dlib::rgb_pixel(255,250,250)
#define    color_black    dlib::rgb_pixel(0,0,0)
#define    color_dim_gray_dim_grey    dlib::rgb_pixel(105,105,105)
#define    color_gray_grey    dlib::rgb_pixel(128,128,128)
#define    color_dark_gray_dark_grey    dlib::rgb_pixel(169,169,169)
#define    color_silver    dlib::rgb_pixel(192,192,192)
#define    color_light_gray_light_grey    dlib::rgb_pixel(211,211,211)
#define    color_gainsboro    dlib::rgb_pixel(220,220,220)
#define    color_white_smoke    dlib::rgb_pixel(245,245,245)
#define    color_white    dlib::rgb_pixel(255,255,255)

//-----------------------------------------------------------

#endif  //  __COLOR_TEXT_H__