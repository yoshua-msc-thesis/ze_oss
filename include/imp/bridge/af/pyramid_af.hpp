#pragma once

#include <vector>
#include <memory>

#include <glog/logging.h>

#include <imp/imgproc/image_pyramid.h>
#include <imp/bridge/af/image_af.hpp>

namespace ze {

template<typename Pixel>
std::shared_ptr<ImagePyramid<Pixel>> createAFImagePyramid(
    const typename ImageAF<Pixel>::Ptr& img_level0,
    FloatType scale_factor=0.5,
    uint32_t max_num_levels=UINT32_MAX,
    uint32_t size_bound=8u)
{
  // Pyramid currently only supports 0.5 scale factor and 8uC1 images.
  CHECK_DOUBLE_EQ(scale_factor, 0.5);
  CHECK(pixel_type<Pixel8uC1>::type == PixelType::i8uC1); // TODO: why does CHECK_EQ not compile?
  using Pyr = ImagePyramid<Pixel>;
  auto pyr = std::make_shared<Pyr>(
        img_level0->size(), scale_factor,
        size_bound, max_num_levels);
  pyr->push_back(img_level0);

  printf("Pyramid levels: %lu\n", pyr->numLevels());

  for (size_t i = 1; i < pyr->numLevels(); ++i)
  {
    const ImageAF<Pixel>& prev = pyr->at(i-1);
    printf("Level %lu, size: %ix%i\n", i-1, prev.width(), prev.height());
    printf("\tAF internal array dims: [%llu, %llu]\n", prev.afArray().dims()[0], prev.afArray().dims()[1]);
    af::array half_sampled = af::resize(0.5f, prev.afArray());
    pyr->emplace_back(std::make_shared<ImageAF<Pixel>>(half_sampled));
  }
  return pyr;
}

} // ze namespace
