// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html.
//
// Copyright (C) 2021 Intel Corporation

#ifndef GAPI_STREAMING_ONEVPL_ONEVPL_FILE_DATA_PROVIDER_HPP
#define GAPI_STREAMING_ONEVPL_ONEVPL_FILE_DATA_PROVIDER_HPP
#include <stdio.h>

#include <opencv2/gapi/streaming/onevpl/data_provider_interface.hpp>

namespace cv {
namespace gapi {
namespace wip {
namespace onevpl {
<<<<<<< HEAD
struct FileDataProvider : public IDataProvider {
=======

// With gcc13, std::unique_ptr(FILE, decltype(&fclose)> causes ignored-attributes warning.
// See https://stackoverflow.com/questions/76849365/can-we-add-attributes-to-standard-function-declarations-without-breaking-standar
#if defined(__GNUC__) && (__GNUC__ >= 13)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"
#endif

struct GAPI_EXPORTS FileDataProvider : public IDataProvider {
>>>>>>> dd08328228f008f270a199b7fb25aab37a91135d

    using file_ptr = std::unique_ptr<FILE, decltype(&fclose)>;
    FileDataProvider(const std::string& file_path);
    ~FileDataProvider();

    size_t fetch_data(size_t out_data_bytes_size, void* out_data) override;
    bool empty() const override;
private:
    file_ptr source_handle;
};

#if defined(__GNUC__) && (__GNUC__ == 13)
#pragma GCC diagnostic pop
#endif

} // namespace onevpl
} // namespace wip
} // namespace gapi
} // namespace cv

#endif // GAPI_STREAMING_ONEVPL_ONEVPL_FILE_DATA_PROVIDER_HPP
