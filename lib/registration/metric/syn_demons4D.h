/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see www.mrtrix.org
 *
 */

#ifndef __registration_metric_syn_demons4D_h__
#define __registration_metric_syn_demons4D_h__

#include "adapter/gradient3D.h"

namespace MR
{
  namespace Registration
  {
    namespace Metric
    {

      template <class Im1ImageType, class Im2ImageType, class Im1MaskType, class Im2MaskType>
      class SyNDemons4D {
        public:
          SyNDemons4D (default_type& global_energy, size_t& global_voxel_count,
                     const Im1ImageType& im1_image, const Im2ImageType& im2_image, const Im1MaskType im1_mask, const Im2MaskType im2_mask) :
                       global_cost (global_energy),
                       global_voxel_count (global_voxel_count),
                       thread_cost (0.0),
                       thread_voxel_count (0),
                       normaliser (0.0),
                       robustness_parameter (-1.e12),
                       intensity_difference_threshold (0.001),
                       denominator_threshold (1e-9),
                       im1_gradient (im1_image, true), im2_gradient (im2_image, true),
                       im1_mask (im1_mask), im2_mask (im2_mask)
          {
            for (size_t d = 0; d < 3; ++d)
              normaliser += im1_image.spacing(d) * im2_image.spacing(d);
            normaliser /= 3.0;
          }

          ~SyNDemons4D () {
            global_cost += thread_cost;
            global_voxel_count += thread_voxel_count;
          }

          void set_im1_mask (const Image<float>& mask) {
            im1_mask = mask;
          }

          void set_im2_mask (const Image<float>& mask) {
            im2_mask = mask;
          }


          void operator() (Im1ImageType& im1_image,
                           Im2ImageType& im2_image,
                           Image<default_type>& im1_update,
                           Image<default_type>& im2_update) {

            assert (im1_image.ndim() == 4 && im2_image.ndim() == 4);

            // Check for boundary condition
            if (im1_image.index(0) == 0 || im1_image.index(0) == im1_image.size(0) - 1 ||
                im1_image.index(1) == 0 || im1_image.index(1) == im1_image.size(1) - 1 ||
                im1_image.index(2) == 0 || im1_image.index(2) == im1_image.size(2) - 1) {
              im1_update.row(3).setZero();
              im2_update.row(3).setZero();
              return;
            }

            typename Im1MaskType::value_type im1_mask_value = 1.0;
            if (im1_mask.valid()) {
              assign_pos_of (im1_image, 0, 3).to (im1_mask);
              im1_mask_value = im1_mask.value();
              if (im1_mask_value < 0.1) {
                im1_update.row(3).setZero();
                im2_update.row(3).setZero();
                return;
              }
            }

            typename Im2MaskType::value_type im2_mask_value = 1.0;
            if (im2_mask.valid()) {
              assign_pos_of (im2_image, 0, 3).to (im2_mask);
              im2_mask_value = im2_mask.value();
              if (im2_mask_value < 0.1) {
                im1_update.row(3).setZero();
                im2_update.row(3).setZero();
                return;
              }
            }

            Eigen::Matrix<typename Im1ImageType::value_type, Eigen::Dynamic, 1> speed =  im2_image.row(3) - im1_image.row(3);
            for (ssize_t i = 0; i < speed.rows(); ++i) {
              if (std::abs (speed[i]) < robustness_parameter)
                speed[i] = 0.0;
            }

            Eigen::Matrix<typename Im1ImageType::value_type, Eigen::Dynamic, 1> speed_squared = speed.array() * speed.array();
            thread_cost += speed_squared.sum();
            thread_voxel_count++;

            assign_pos_of (im1_image, 0, 3).to (im2_gradient);
            assign_pos_of (im2_image, 0, 3).to (im1_gradient);

            Eigen::Matrix<typename Im1ImageType::value_type, 3, Eigen::Dynamic> grad = (im2_gradient.row(3) + im1_gradient.row(3)).array() / 2.0;
//            Eigen::Matrix<typename Im1ImageType::value_type, 1, Eigen::Dynamic> denominator = speed_squared.array() / normaliser + grad.colwise().squaredNorm();

//            Eigen::Vector3 total_update = Eigen::Vector3::Zero();
//            for (size_t i = 0; i < im2_image.size(3); ++i) {
//              if (!(std::abs (speed[i]) < intensity_difference_threshold || denominator[i] < denominator_threshold))
//                total_update += speed[i] * (grad.array().col(i) / denominator);
//            }
//            total_update = total_update.array() / im2_image.size(3);
//            im1_update.row(3) = total_update;
//            im2_update.row(3) = -total_update;
          }

          protected:
            default_type& global_cost;
            size_t& global_voxel_count;
            default_type thread_cost;
            size_t thread_voxel_count;
            default_type normaliser;
            const default_type robustness_parameter;
            const default_type intensity_difference_threshold;
            const default_type denominator_threshold;

            Adapter::Gradient3D<Im1ImageType> im1_gradient;
            Adapter::Gradient3D<Im2ImageType> im2_gradient;
            Im1MaskType im1_mask;
            Im2MaskType im2_mask;

      };
    }
  }
}
#endif
