/*
* Copyright (c) 2014, Autonomous Systems Lab
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
* * Redistributions of source code must retain the above copyright
* notice, this list of conditions and the following disclaimer.
* * Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution.
* * Neither the name of the Autonomous Systems Lab, ETH Zurich nor the
* names of its contributors may be used to endorse or promote products
* derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/


#include <memory>

#include <Eigen/StdVector>
#include <glog/logging.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <ros/ros.h>
#include <ros/package.h>
#include <geometry_msgs/Pose.h>
#pragma GCC diagnostic pop

#include "rovio/CameraCalibration.hpp"
#include "rovio/FilterConfiguration.hpp"
#include "rovio/Memory.hpp"
#include "rovio/RovioFilter.hpp"
#include "rovio/RovioRosNode.hpp"
#ifdef MAKE_SCENE
#include "rovio/RovioScene.hpp"
#endif

#ifdef ROVIO_NMAXFEATURE
static constexpr int nMax_ = ROVIO_NMAXFEATURE;
#else
static constexpr int nMax_ = 25; // Maximal number of considered features in the filter state.
#endif

#ifdef ROVIO_NLEVELS
static constexpr int nLevels_ = ROVIO_NLEVELS;
#else
static constexpr int nLevels_ = 4; // // Total number of pyramid levels considered.
#endif

#ifdef ROVIO_PATCHSIZE
static constexpr int patchSize_ = ROVIO_PATCHSIZE;
#else
static constexpr int patchSize_ = 6; // Edge length of the patches (in pixel). Must be a multiple of 2!
#endif

#ifdef ROVIO_NCAM
static constexpr int nCam_ = ROVIO_NCAM;
#else
static constexpr int nCam_ = 1; // Used total number of cameras.
#endif

#ifdef ROVIO_NPOSE
static constexpr int nPose_ = ROVIO_NPOSE;
#else
static constexpr int nPose_ = 0; // Additional pose states.
#endif

typedef rovio::RovioFilter<rovio::FilterState<nMax_,nLevels_,patchSize_,nCam_,nPose_>> mtFilter;

#ifdef MAKE_SCENE
static rovio::RovioScene<mtFilter> mRovioScene;

void idleFunc(){
  ros::spinOnce();
  mRovioScene.drawScene(mRovioScene.mpFilter_->safe_);
}
#endif

int main(int argc, char** argv){
  ros::init(argc, argv, "rovio");
  google::InitGoogleLogging(argv[0]);

  ros::NodeHandle nh;
  ros::NodeHandle nh_private("~");

  // Load filter configuration.
  std::string rootdir = ros::package::getPath("rovio"); // Leaks memory
  std::string filter_config_file = rootdir + "/cfg/rovio.info";
  nh_private.param("filter_config", filter_config_file, filter_config_file);
  rovio::FilterConfiguration filter_config(filter_config_file);

  // Overwrite camera calibrations that are part of the filter configuration if
  // there is a ros parameter providing a different camera calibration file.
  rovio::CameraCalibrationVector camera_calibrations(nCam_);
  for (unsigned int camID = 0; camID < nCam_; ++camID) {
    std::string camera_config;
    if (nh_private.getParam("camera" + std::to_string(camID)
                            + "_config", camera_config)) {
      camera_calibrations[camID].loadFromFile(camera_config);
    }
  }

  // Set up ROVIO by using the RovioInterface.
  rovio::RovioInterfaceImpl<mtFilter> rovioInterface(filter_config, camera_calibrations);
  rovioInterface.makeTest();

  // Create the ROVIO ROS node and connect it to the interface.
  rovio::RovioRosNode<mtFilter> rovioNode(nh, nh_private, &rovioInterface);

#ifdef MAKE_SCENE
  // Scene
  std::string mVSFileName = rootdir + "/shaders/shader.vs";
  std::string mFSFileName = rootdir + "/shaders/shader.fs";
  mRovioScene.initScene(argc,argv,mVSFileName,mFSFileName,mpFilter);
  mRovioScene.setIdleFunction(idleFunc);
  mRovioScene.addKeyboardCB('r',[&rovioNode]() mutable {rovioNode.requestReset();});
  glutMainLoop();
#else
  ros::spin();
#endif
  return 0;
}
