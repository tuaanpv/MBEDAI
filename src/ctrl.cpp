/**********************************************************************************
*   Created 2018 by Johny Pham
*   tuaanpv@gmail.com
*   Youtube/Tiktok/Instagram: tuaanpv
***********************************************************************************/

#include  "ctrl.h"

// Controller::is_using_cuda()
// {

//     int using_cuda = 0; // return 0 if CUDA and cuDNN not found
//                         // return 0x00000001 if CUDA found
//                         // return 0x00000010 if cuDNN found

//     enum e_using_cuda
//     {
//         NOT_CUDA = 0,
//         USE_CUDA = 1,
//         USE_CUDNN =2
//     };

//     // std::cout << cv::getBuildInformation();
//     std::string build_info = cv::getBuildInformation();

//     size_t found = build_info.find("NVIDIA CUDA:");
//     if(found != std::string::npos)
//     {
//         if((build_info.find("YES", found ) - found) <= 33);
//         {
//             using_cuda |= 0x01;
//             // std::cout << "OpenCV uses NVIDIA CUDA: YES" << std::endl;
//         }
//     }

//     found = build_info.find("cuDNN:");
//     if(found != std::string::npos)
//     {
//         if((build_info.find("YES", found ) - found) <= 33);
//         {
//             using_cuda |= 0x02;
//             // std::cout << "OpenCV uses cuDNN: YES" << std::endl;
//         }
//     }

//     if(using_cuda && USE_CUDA)
//     {
//         std::cout << "OpenCV uses NVIDIA CUDA: YES" << std::endl;
//     }
//     if(using_cuda && USE_CUDNN)
//     {
//         std::cout << "OpenCV uses cuDNN: YES" << std::endl;
//     }

//     return using_cuda;
// }