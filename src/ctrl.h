/**********************************************************************************
*   Created 2018 by Johny Pham
*   tuaanpv@gmail.com
*   Youtube/Tiktok/Instagram: tuaanpv
***********************************************************************************/

#ifndef __CTRL_H__
#define __CTRL_H__

#include <opencv2/dnn.hpp>
#include <queue>

#include "Buffer.h"
#include "color_text.h"


#include <dlib/opencv.h>
#include <dlib/opencv/cv_image.h>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/render_face_detections.h>
#include <dlib/svm_threaded.h>
#include <dlib/gui_widgets.h>
#include <dlib/image_processing.h>
#include <dlib/data_io.h>
#include <dlib/image_transforms.h>
#include <dlib/cmd_line_parser.h>
#include <dlib/dir_nav.h>
#include <dlib/clustering.h>
#include <dlib/string.h>
#include <dlib/dnn.h>
#include <dlib/image_io.h>
#include <dlib/matrix.h>
#include <dlib/sparse_vector.h>
#include <dlib/geometry.h>
#include <dlib/queue.h>

#include <time.h>


// #define CTRL_STOP   0
// #define CTRL_RUN    1
// #define CTRL_IDLE   2
// #define CTRL_KILL   4

// class Controller
// {
//     public:
//         int is_using_cuda();

//     private:
//         int using_cuda;
// };


#endif