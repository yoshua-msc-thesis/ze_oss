#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <imp/core/image_base.hpp>
#include <ze/common/macros.h>
#include <ze/common/types.h>
#include <ze/data_provider/data_provider_base.hpp>

// fwd
namespace cv {
class Mat;
}

namespace ze {

//fwd
namespace internal {
struct MeasurementBase;
}

class DataProviderCsv : public DataProviderBase
{
public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW

  using StampMeasurementPair = std::pair<int64_t, std::shared_ptr<internal::MeasurementBase>> ;
  using DataBuffer = std::multimap<int64_t, std::shared_ptr<internal::MeasurementBase>> ;

  DataProviderCsv(
      const std::string& csv_directory,
      const std::map<std::string, size_t>& imu_topics,
      const std::map<std::string, size_t>& camera_topics);

  virtual ~DataProviderCsv() = default;

  virtual bool spinOnce() override;

  virtual bool ok() const override;

  virtual size_t imuCount() const;

  virtual size_t cameraCount() const;

  inline size_t size() const
  {
    return buffer_.size();
  }

private:

  void loadImuData(
      const std::string data_dir,
      const size_t imu_index,
      const int64_t playback_delay);

  void loadCameraData(
      const std::string& data_dir,
      const size_t camera_index,
      int64_t playback_delay);

  //! Buffer to chronologically sort the data.
  DataBuffer buffer_;

  //! Points to the next published buffer value. Buffer can't change once loaded!
  DataBuffer::const_iterator buffer_it_;

  std::map<std::string, size_t> imu_topics_;
  std::map<std::string, size_t> camera_topics_;

  size_t imu_count_ = 0u;
};

} // namespace ze
