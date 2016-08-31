#include <ros/ros.h>
#include <image_transport/image_transport.h>

#include <dynamic_reconfigure/server.h>
#include <imp_ros_rof_denoising/RofNodeConfig.h>

#include <opencv2/highgui/highgui.hpp>
#include <cv_bridge/cv_bridge.h>

#include <imp/bridge/opencv/image_cv.hpp>
#include <imp/cu_imgproc/cu_rof_denoising.cuh>
#include <imp/bridge/opencv/cu_cv_bridge.hpp>
#include <imp/cu_core/cu_utils.hpp>

#include <sensor_msgs/Image.h>


namespace ze {

//------------------------------------------------------------------------------
class RofNode
{
public:
  RofNode() = default;
  ~RofNode() = default;

  void imgCb(const sensor_msgs::ImageConstPtr& img_msg);

  void paramCb(imp_ros_denoising::RofNodeConfig& config, uint32_t level);


private:
  ze::cu::RofDenoising8uC1::Ptr rof_;
  ze::ImageCv8uC1::Ptr cv_img_;
  ze::ImageCv8uC1::Ptr cv_denoised_;
  ze::cu::ImageGpu8uC1::Ptr img_;
  ze::cu::ImageGpu8uC1::Ptr denoised_;
};

//------------------------------------------------------------------------------
void RofNode::imgCb(const sensor_msgs::ImageConstPtr &img_msg)
{
  cv::Mat mat;
  try
  {
    mat = cv_bridge::toCvShare(img_msg, "mono8")->image;
  }
  catch (std::exception& e)
  {
    ROS_ERROR("Could not extract image from input message. Exception: %s", e.what());
    return;
  }

  ze::Size2u im_size((std::uint32_t)mat.cols, (std::uint32_t)mat.rows);

  if (!rof_ || !cv_img_ || !img_ || im_size != cv_img_->size())
  {
    rof_.reset(new ze::cu::RofDenoising8uC1());
//    rof_->init(im_size);

    cv_img_.reset(new ze::ImageCv8uC1(im_size));
    cv_denoised_.reset(new ze::ImageCv8uC1(im_size));
    img_.reset(new ze::cu::ImageGpu8uC1(im_size));
    denoised_.reset(new ze::cu::ImageGpu8uC1(im_size));
  }

  cv_img_->cvMat() = mat;
  img_->copyFrom(*cv_img_);
  rof_->denoise(denoised_, img_);
  denoised_->copyTo(*cv_denoised_);
  cv::imshow("input", cv_img_->cvMat());
  cv::imshow("denoised", cv_denoised_->cvMat());
  cv::waitKey(1);
}

//------------------------------------------------------------------------------
void RofNode::paramCb(imp_ros_denoising::RofNodeConfig& config, uint32_t level)
{
  if(!rof_)
    return;
  rof_->params().lambda = ze::cu::max(1e-6, config.lambda);
}


}

//==============================================================================
int main(int argc, char **argv)
{
  ros::init(argc, argv, "RofNode");


  ros::NodeHandle nh;
  ROS_INFO("testing the RofNode");
  ze::RofNode rof_node;

  // reconfigure stuff
  dynamic_reconfigure::Server<imp_ros_denoising::RofNodeConfig> server;
  dynamic_reconfigure::Server<imp_ros_denoising::RofNodeConfig>::CallbackType f;
  f = boost::bind(&ze::RofNode::paramCb, &rof_node, _1, _2);
  server.setCallback(f);


  image_transport::ImageTransport it(nh);
  image_transport::Subscriber sub = it.subscribe(
        "camera/image_raw", 1, &ze::RofNode::imgCb, &rof_node);

  ros::spin();
  return EXIT_SUCCESS;
}
