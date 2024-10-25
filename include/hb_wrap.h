#ifndef OPTIFT_HB_WRAP_H
#define OPTIFT_HB_WRAP_H

#include <memory>

#include <hb-ot.h>
#include <hb-subset.h>
#include <hb.h>

namespace optift {

class SubsetInputPtr
    : public std::unique_ptr<hb_subset_input_t,
                             decltype(&hb_subset_input_destroy)> {
  public:
    SubsetInputPtr()
        : std::unique_ptr<hb_subset_input_t,
                          decltype(&hb_subset_input_destroy)>{
              hb_subset_input_create_or_fail(), hb_subset_input_destroy} {
        if (this->get() == nullptr) {
            throw std::runtime_error("failed to create subset input object");
        }
    }
};

class BlobPtr : public std::unique_ptr<hb_blob_t, decltype(&hb_blob_destroy)> {
  public:
    explicit BlobPtr(hb_blob_t *blob)
        : std::unique_ptr<hb_blob_t, decltype(&hb_blob_destroy)>{
              blob, hb_blob_destroy} {
        if (this->get() == nullptr) {
            throw std::runtime_error("failed to create blob object");
        }
    }
};

class FacePtr : public std::unique_ptr<hb_face_t, decltype(&hb_face_destroy)> {
  public:
    explicit FacePtr(hb_face_t *face)
        : std::unique_ptr<hb_face_t, decltype(&hb_face_destroy)>{
              face, hb_face_destroy} {
        if (this->get() == nullptr) {
            throw std::runtime_error("failed to create face object");
        }
    }
};

} // namespace optift

#endif