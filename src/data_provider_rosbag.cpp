#include <ze/data_provider/data_provider_rosbag.h>

#include <ze/common/logging.hpp>
#include <rosbag/query.h>
#include <sensor_msgs/Image.h>
#include <sensor_msgs/Imu.h>
#include <sensor_msgs/image_encodings.h>
#include <cv_bridge/cv_bridge.h>

#include <ze/common/time_conversions.h>
#include <ze/common/string_utils.h>
#include <ze/common/path_utils.h>
#include <imp/bridge/opencv/image_cv.hpp>

DEFINE_int32(data_source_stop_after_n_frames, -1,
             "How many frames should be processed?");

namespace ze {

DataProviderRosbag::DataProviderRosbag(
    const std::string& bag_filename,
    const std::string& imu_topic,
    const std::map<std::string, size_t>& img_topic_camidx_map)
  : DataProviderBase(DataProviderType::Rosbag)
  , img_topic_camidx_map_(img_topic_camidx_map)
  , imu_topic_(imu_topic)
{

  // TODO(cfo): Check if topics exists.
  // TODO(cfo): Display number of messages per topic in the beginning.

  CHECK(fileExists(bag_filename));
  bag_.reset(new rosbag::Bag);
  try
  {
    bag_->open(bag_filename, rosbag::bagmode::Read);
  }
  catch(const std::exception e)
  {
    LOG(FATAL) << "Could not open rosbag " << bag_filename << ": " << e.what();
  }

  std::vector<std::string> topics;
  for(auto it : img_topic_camidx_map_)
  {
    topics.push_back(it.first);
  }
  if(!imu_topic.empty())
  {
    topics.push_back(imu_topic);
  }

  bag_view_.reset(new rosbag::View(*bag_, rosbag::TopicQuery(topics)));
  bag_view_it_ = bag_view_->begin();
}

void DataProviderRosbag::spin()
{
  while(ok() && !quit_)
  {
    spinOnce();
  }
}

bool DataProviderRosbag::spinOnce()
{
  if(bag_view_it_ != bag_view_->end())
  {
    const rosbag::MessageInstance m = *bag_view_it_;

    sensor_msgs::ImageConstPtr m_img = m.instantiate<sensor_msgs::Image>();
    if(m_img && camera_callback_)
    {
      auto it = img_topic_camidx_map_.find(m.getTopic());
      if(it != img_topic_camidx_map_.end())
      {
        ++n_processed_images_;
        if (FLAGS_data_source_stop_after_n_frames > 0
            && n_processed_images_ > FLAGS_data_source_stop_after_n_frames)
        {
          LOG(WARNING) << "Data source has reached max number of desired frames.";
          quit_ = true;
          return false;
        }

        cv::Mat img;
        try
        {
          img = cv_bridge::toCvCopy(m_img, "mono8")->image;
        }
        catch (cv_bridge::Exception& e)
        {
          LOG(FATAL) << "Could not read image: " << e.what();
        }
        auto img_ptr = std::make_shared<ImageCv8uC1>(img);
        camera_callback_(m_img->header.stamp.toNSec(), img_ptr, it->second);
      }
      else
      {
        LOG_FIRST_N(WARNING, 1) << "Topic in bag that is not subscribed: " << m.getTopic();
      }
    }
    else if(m_img && !camera_callback_)
    {
      LOG_FIRST_N(WARNING, 1) << "No camera callback registered but measurements available";
    }

    const sensor_msgs::ImuConstPtr m_imu = m.instantiate<sensor_msgs::Imu>();
    if(m_imu && imu_callback_)
    {
      if(m.getTopic() == imu_topic_)
      {
        const Eigen::Vector3d gyr(
              m_imu->angular_velocity.x,
              m_imu->angular_velocity.y,
              m_imu->angular_velocity.z);
        const Eigen::Vector3d acc(
              m_imu->linear_acceleration.x,
              m_imu->linear_acceleration.y,
              m_imu->linear_acceleration.z);
        int64_t stamp = m_imu->header.stamp.toNSec();
        CHECK_GT(stamp, last_imu_stamp_);
        imu_callback_(stamp, acc, gyr);
        last_imu_stamp_ = stamp;
      }
      else
      {
        LOG_FIRST_N(WARNING, 1) << "Topic in bag that is not subscribed: " << m.getTopic();
      }
    }
    else if(m_imu && !imu_callback_)
    {
      LOG_FIRST_N(WARNING, 1) << "No IMU callback registered but measurements available";
    }

    ++bag_view_it_;
    return true;
  }
  return false;
}

bool DataProviderRosbag::ok() const
{
  if (!running_)
  {
    VLOG(1) << "Data Provider was terminated.";
    return false;
  }
  if (bag_view_it_ == bag_view_->end())
  {
    VLOG(1) << "All data processed.";
    return false;
  }
  return true;
}

size_t DataProviderRosbag::size() const
{
  CHECK(bag_view_);
  return bag_view_->size();
}

} // namespace ze
