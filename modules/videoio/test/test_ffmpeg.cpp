// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html.

#include "test_precomp.hpp"

using namespace std;

namespace opencv_test { namespace {

static inline long long getFileSize(const string &filename)
{
    ifstream f(filename, ios_base::in | ios_base::binary);
    f.seekg(0, ios_base::end);
    return f.tellg();
}

typedef tuple<string, string, Size> FourCC_Ext_Size;
typedef testing::TestWithParam< FourCC_Ext_Size > videoio_ffmpeg;

TEST_P(videoio_ffmpeg, write_big)
{
    if (!videoio_registry::hasBackend(CAP_FFMPEG))
        throw SkipTestException("FFmpeg backend was not found");

    const string fourcc = get<0>(GetParam());
    const string ext = get<1>(GetParam());
    const Size sz = get<2>(GetParam());
    const double time_sec = 1;
    const double fps = 25;

    ostringstream buf;
    buf << "write_big_" << fourcc << "." << ext;
    const string filename = tempfile(buf.str().c_str());

    VideoWriter writer(filename, CAP_FFMPEG, fourccFromString(fourcc), fps, sz);
    if (ext == "mp4" && fourcc == "H264" && !writer.isOpened())
    {
        throw cvtest::SkipTestException("H264/mp4 codec is not supported - SKIP");
    }
    ASSERT_TRUE(writer.isOpened());
    Mat img(sz, CV_8UC3, Scalar::all(0));
    const int coeff = cvRound(min(sz.width, sz.height)/(fps * time_sec));
    for (int i = 0 ; i < static_cast<int>(fps * time_sec); i++ )
    {
        rectangle(img,
                  Point2i(coeff * i, coeff * i),
                  Point2i(coeff * (i + 1), coeff * (i + 1)),
                  Scalar::all(255 * (1.0 - static_cast<double>(i) / (fps * time_sec * 2))),
                  -1);
        writer << img;
    }
    writer.release();
    EXPECT_GT(getFileSize(filename), 8192);
    remove(filename.c_str());
}

static const Size bigSize(4096, 4096);

const FourCC_Ext_Size entries[] =
{
    make_tuple("", "avi", bigSize),
    make_tuple("DX50", "avi", bigSize),
    make_tuple("FLV1", "avi", bigSize),
    make_tuple("H261", "avi", Size(352, 288)),
    make_tuple("H263", "avi", Size(704, 576)),
    make_tuple("I420", "avi", bigSize),
    make_tuple("MJPG", "avi", bigSize),
    make_tuple("mp4v", "avi", bigSize),
    make_tuple("MPEG", "avi", Size(720, 576)),
    make_tuple("XVID", "avi", bigSize),
    make_tuple("H264", "mp4", Size(4096, 2160))
};

INSTANTIATE_TEST_CASE_P(videoio, videoio_ffmpeg, testing::ValuesIn(entries));

//==========================================================================

TEST(videoio_ffmpeg, image)
{
    if (!videoio_registry::hasBackend(CAP_FFMPEG))
        throw SkipTestException("FFmpeg backend was not found");

    const string filename = findDataFile("readwrite/ordinary.bmp");
    Mat image = imread(filename, IMREAD_COLOR);
    ASSERT_FALSE(image.empty());
    VideoCapture cap(filename, CAP_FFMPEG);
    ASSERT_TRUE(cap.isOpened());
    Mat frame1, frame2;
    cap >> frame1 >> frame2;
    ASSERT_FALSE(frame1.empty());
    ASSERT_TRUE(frame2.empty());
    ASSERT_EQ(0, cvtest::norm(image, frame1, NORM_INF));
}

//==========================================================================

typedef tuple<VideoCaptureAPIs, string, string, string, string, string> videoio_container_params_t;
typedef testing::TestWithParam< videoio_container_params_t > videoio_container;

TEST_P(videoio_container, read)
{
    const VideoCaptureAPIs api = get<0>(GetParam());

    if (!videoio_registry::hasBackend(api))
        throw SkipTestException("Backend was not found");

    const string path = get<1>(GetParam());
    const string ext = get<2>(GetParam());
    const string ext_raw = get<3>(GetParam());
    const string codec = get<4>(GetParam());
    const string pixelFormat = get<5>(GetParam());
    const string fileName = path + "." + ext;
    const string fileNameOut = tempfile(cv::format("test_container_stream.%s", ext_raw.c_str()).c_str());

    // Write encoded video read using VideoContainer to tmp file
    size_t totalBytes = 0;
    {
        VideoCapture container(findDataFile(fileName), api);
        if (!container.isOpened())
            throw SkipTestException("Video stream is not supported");
        if (!container.set(CAP_PROP_FORMAT, -1))  // turn off video decoder (extract stream)
            throw SkipTestException("Fetching of RAW video streams is not supported");
        ASSERT_EQ(-1.f, container.get(CAP_PROP_FORMAT));  // check
        EXPECT_EQ(codec, fourccToString((int)container.get(CAP_PROP_FOURCC)));
        EXPECT_EQ(pixelFormat, fourccToString((int)container.get(CAP_PROP_CODEC_PIXEL_FORMAT)));

        std::ofstream file(fileNameOut.c_str(), ios::out | ios::trunc | std::ios::binary);
        Mat raw_data;
        while (true)
        {
            container >> raw_data;
            size_t size = raw_data.total();
            if (raw_data.empty())
                break;
            ASSERT_EQ(CV_8UC1, raw_data.type());
            ASSERT_LE(raw_data.dims, 2);
            ASSERT_EQ(raw_data.rows, 1);
            ASSERT_EQ((size_t)raw_data.cols, raw_data.total());
            ASSERT_TRUE(raw_data.isContinuous());
            totalBytes += size;
            file.write(reinterpret_cast<char*>(raw_data.data), size);
            ASSERT_FALSE(file.fail());
        }
        ASSERT_GE(totalBytes, (size_t)65536) << "Encoded stream is too small";
    }

    std::cout << "Checking extracted video stream: " << fileNameOut << " (size: " << totalBytes << " bytes)" << std::endl;

    // Check decoded frames read from original media are equal to frames decoded from tmp file
    {
        VideoCapture capReference(findDataFile(fileName), api);
        ASSERT_TRUE(capReference.isOpened());
        VideoCapture capActual(fileNameOut.c_str(), api);
        ASSERT_TRUE(capActual.isOpened());
        Mat reference, actual;
        int nframes = 0, n_err = 0;
        while (capReference.read(reference) && n_err < 3)
        {
            nframes++;
            ASSERT_TRUE(capActual.read(actual)) << nframes;
            EXPECT_EQ(0, cvtest::norm(actual, reference, NORM_INF)) << "frame=" << nframes << " err=" << ++n_err;
        }
        ASSERT_GT(nframes, 0);
    }

    ASSERT_EQ(0, remove(fileNameOut.c_str()));
}

const videoio_container_params_t videoio_container_params[] =
{
    videoio_container_params_t(CAP_FFMPEG, "video/big_buck_bunny", "h264", "h264", "h264", "I420"),
    videoio_container_params_t(CAP_FFMPEG, "video/big_buck_bunny", "h265", "h265", "hevc", "I420"),
    videoio_container_params_t(CAP_FFMPEG, "video/big_buck_bunny", "mjpg.avi", "mjpg", "MJPG", "I420"),
    //videoio_container_params_t(CAP_FFMPEG, "video/big_buck_bunny", "h264.mkv", "mkv.h264", "h264", "I420"),
    //videoio_container_params_t(CAP_FFMPEG, "video/big_buck_bunny", "h265.mkv", "mkv.h265", "hevc", "I420"),
    //videoio_container_params_t(CAP_FFMPEG, "video/big_buck_bunny", "h264.mp4", "mp4.avc1", "avc1", "I420"),
    //videoio_container_params_t(CAP_FFMPEG, "video/big_buck_bunny", "h265.mp4", "mp4.hev1", "hev1", "I420"),
};

INSTANTIATE_TEST_CASE_P(/**/, videoio_container, testing::ValuesIn(videoio_container_params));

<<<<<<< HEAD
=======
typedef tuple<VideoCaptureAPIs, string, int, int, int, int, int> videoio_container_get_params_t;
typedef testing::TestWithParam<videoio_container_get_params_t > videoio_container_get;

TEST_P(videoio_container_get, read)
{
    const VideoCaptureAPIs api = get<0>(GetParam());

    if (!videoio_registry::hasBackend(api))
        throw SkipTestException("Backend was not found");

    const string fileName = get<1>(GetParam());
    const int height = get<2>(GetParam());
    const int width = get<3>(GetParam());
    const int nFrames = get<4>(GetParam());
    const int bitrate = get<5>(GetParam());
    const int fps = get<6>(GetParam());

    VideoCapture container(findDataFile(fileName), api, { CAP_PROP_FORMAT, -1 });
    if (!container.isOpened())
        throw SkipTestException("Video stream is not supported");

    const int heightProp = static_cast<int>(container.get(CAP_PROP_FRAME_HEIGHT));
    ASSERT_EQ(height, heightProp);
    const int widthProp = static_cast<int>(container.get(CAP_PROP_FRAME_WIDTH));
    ASSERT_EQ(width, widthProp);
    const int nFramesProp = static_cast<int>(container.get(CAP_PROP_FRAME_COUNT));
    ASSERT_EQ(nFrames, nFramesProp);
    const int bitrateProp = static_cast<int>(container.get(CAP_PROP_BITRATE));
    ASSERT_EQ(bitrate, bitrateProp);
    const double fpsProp = container.get(CAP_PROP_FPS);
    ASSERT_EQ(fps, fpsProp);

    vector<int> displayTimeMs;
    int iFrame = 1;
    while (container.grab()) {
        displayTimeMs.push_back(static_cast<int>(container.get(CAP_PROP_POS_MSEC)));
        const int iFrameProp = static_cast<int>(container.get(CAP_PROP_POS_FRAMES));
        ASSERT_EQ(iFrame++, iFrameProp);
    }
    sort(displayTimeMs.begin(), displayTimeMs.end());
    vector<int> displayTimeDiffMs(displayTimeMs.size());
    std::adjacent_difference(displayTimeMs.begin(), displayTimeMs.end(), displayTimeDiffMs.begin());
    auto minTimeMsIt = min_element(displayTimeDiffMs.begin() + 1, displayTimeDiffMs.end());
    auto maxTimeMsIt = max_element(displayTimeDiffMs.begin() + 1, displayTimeDiffMs.end());
    const int frameTimeMs = static_cast<int>(1000.0 / fps);
    ASSERT_NEAR(frameTimeMs, *minTimeMsIt, 1);
    ASSERT_NEAR(frameTimeMs, *maxTimeMsIt, 1);
}

const videoio_container_get_params_t videoio_container_get_params[] =
{
    videoio_container_get_params_t(CAP_FFMPEG, "video/big_buck_bunny.mp4", 384, 672, 125, 483, 24),
    videoio_container_get_params_t(CAP_FFMPEG, "video/big_buck_bunny.mjpg.avi", 384, 672, 125, 2713, 24),
    videoio_container_get_params_t(CAP_FFMPEG, "video/sample_322x242_15frames.yuv420p.libx264.mp4", 242, 322, 15, 542, 25)
};

INSTANTIATE_TEST_CASE_P(/**/, videoio_container_get, testing::ValuesIn(videoio_container_get_params));

typedef tuple<string, string, int, int, bool, bool> videoio_encapsulate_params_t;
typedef testing::TestWithParam< videoio_encapsulate_params_t > videoio_encapsulate;

#if defined(WIN32)  // remove when FFmpeg wrapper includes PR25874
#define WIN32_WAIT_FOR_FFMPEG_WRAPPER_UPDATE
#endif

TEST_P(videoio_encapsulate, write)
{
    const VideoCaptureAPIs api = CAP_FFMPEG;
    if (!videoio_registry::hasBackend(api))
        throw SkipTestException("FFmpeg backend was not found");

    const string fileName = findDataFile(get<0>(GetParam()));
    const string ext = get<1>(GetParam());
    const int idrPeriod = get<2>(GetParam());
    const int nFrames = get<3>(GetParam());
    const string fileNameOut = tempfile(cv::format("test_encapsulated_stream.%s", ext.c_str()).c_str());
    const bool setPts = get<4>(GetParam());
    const bool tsWorking = get<5>(GetParam());

    // Use VideoWriter to encapsulate encoded video read with VideoReader
    {
        VideoCapture capRaw(fileName, api, { CAP_PROP_FORMAT, -1 });
        ASSERT_TRUE(capRaw.isOpened());
        const int width = static_cast<int>(capRaw.get(CAP_PROP_FRAME_WIDTH));
        const int height = static_cast<int>(capRaw.get(CAP_PROP_FRAME_HEIGHT));
        const double fps = capRaw.get(CAP_PROP_FPS);
        const int codecExtradataIndex = static_cast<int>(capRaw.get(CAP_PROP_CODEC_EXTRADATA_INDEX));
        Mat extraData;
        capRaw.retrieve(extraData, codecExtradataIndex);
        const int fourcc = static_cast<int>(capRaw.get(CAP_PROP_FOURCC));
        const bool mpeg4 = (fourcc == fourccFromString("FMP4"));
        VideoWriter container(fileNameOut, api, fourcc, fps, { width, height }, { VideoWriterProperties::VIDEOWRITER_PROP_RAW_VIDEO, 1, VideoWriterProperties::VIDEOWRITER_PROP_KEY_INTERVAL, idrPeriod });
        ASSERT_TRUE(container.isOpened());
        Mat rawFrame;
        for (int i = 0; i < nFrames; i++) {
            ASSERT_TRUE(capRaw.read(rawFrame));
#if !defined(WIN32_WAIT_FOR_FFMPEG_WRAPPER_UPDATE)
            if (setPts && i == 0) {
                ASSERT_TRUE(container.set(VIDEOWRITER_PROP_DTS_DELAY, capRaw.get(CAP_PROP_DTS_DELAY)));
            }
#endif
            ASSERT_FALSE(rawFrame.empty());
            if (i == 0 && mpeg4) {
                Mat tmp = rawFrame.clone();
                const size_t newSzt = tmp.total() + extraData.total();
                const int newSz = static_cast<int>(newSzt);
                ASSERT_TRUE(newSzt == static_cast<size_t>(newSz));
                rawFrame = Mat(1, newSz, CV_8UC1);
                memcpy(rawFrame.data, extraData.data, extraData.total());
                memcpy(rawFrame.data + extraData.total(), tmp.data, tmp.total());
            }
#if !defined(WIN32_WAIT_FOR_FFMPEG_WRAPPER_UPDATE)
            if (setPts) {
                ASSERT_TRUE(container.set(VIDEOWRITER_PROP_PTS, capRaw.get(CAP_PROP_PTS)));
            }
#endif
            container.write(rawFrame);
        }
        container.release();
    }

    std::cout << "Checking encapsulated video container: " << fileNameOut << std::endl;

    // Check encapsulated video container is "identical" to the original
    {
        VideoCapture capReference(fileName), capActual(fileNameOut), capActualRaw(fileNameOut, api, { CAP_PROP_FORMAT, -1 });
        ASSERT_TRUE(capReference.isOpened());
        ASSERT_TRUE(capActual.isOpened());
        ASSERT_TRUE(capActualRaw.isOpened());
        const double fpsReference = capReference.get(CAP_PROP_FPS);
        const double fpsActual = capActual.get(CAP_PROP_FPS);
        ASSERT_NEAR(fpsReference, fpsActual, 1e-2);
        const int nFramesActual = static_cast<int>(capActual.get(CAP_PROP_FRAME_COUNT));
        ASSERT_EQ(nFrames, nFramesActual);

        Mat reference, actual;
        for (int i = 0; i < nFrames; i++) {
            ASSERT_TRUE(capReference.read(reference));
            ASSERT_FALSE(reference.empty());
            ASSERT_TRUE(capActual.read(actual));
            ASSERT_FALSE(actual.empty());
            ASSERT_EQ(0, cvtest::norm(reference, actual, NORM_INF));
            ASSERT_TRUE(capActualRaw.grab());
            const bool keyFrameActual = capActualRaw.get(CAP_PROP_LRF_HAS_KEY_FRAME) == 1.;
            const bool keyFrameReference = idrPeriod ? i % idrPeriod == 0 : 1;
            ASSERT_EQ(keyFrameReference, keyFrameActual);
#if !defined(WIN32_WAIT_FOR_FFMPEG_WRAPPER_UPDATE)
            if (tsWorking) {
                ASSERT_EQ(round(capReference.get(CAP_PROP_POS_MSEC)), round(capActual.get(CAP_PROP_POS_MSEC)));
            }
#endif
        }
    }

    ASSERT_EQ(0, remove(fileNameOut.c_str()));
}

const videoio_encapsulate_params_t videoio_encapsulate_params[] =
{
    videoio_encapsulate_params_t("video/big_buck_bunny.h264", "avi", 125, 125, false, false), // tsWorking = false: no timestamp information
    videoio_encapsulate_params_t("video/big_buck_bunny.h265", "mp4", 125, 125, false, false), // tsWorking = false: no timestamp information
    videoio_encapsulate_params_t("video/big_buck_bunny.wmv", "wmv", 12, 13, false, true),
    videoio_encapsulate_params_t("video/big_buck_bunny.mp4", "mp4", 12, 13, false, true),
    videoio_encapsulate_params_t("video/big_buck_bunny.mjpg.avi", "mp4", 0, 4, false, true),
    videoio_encapsulate_params_t("video/big_buck_bunny.mov", "mp4", 12, 13, false, true),
    videoio_encapsulate_params_t("video/big_buck_bunny.avi", "mp4", 125, 125, false, false), // tsWorking = false: PTS not available for all frames
    videoio_encapsulate_params_t("video/big_buck_bunny.mpg", "mp4", 12, 13, true, true),
    videoio_encapsulate_params_t("video/VID00003-20100701-2204.wmv", "wmv", 12, 13, false, true),
    videoio_encapsulate_params_t("video/VID00003-20100701-2204.mpg", "mp4", 12, 13, false, false), // tsWorking = false: PTS not available for all frames
    videoio_encapsulate_params_t("video/VID00003-20100701-2204.avi", "mp4", 12, 13, false, false), // tsWorking = false: Unable to correctly set PTS when writing
    videoio_encapsulate_params_t("video/VID00003-20100701-2204.3GP", "mp4", 51, 52, false, false), // tsWorking = false: Source with variable fps
    videoio_encapsulate_params_t("video/sample_sorenson.avi", "mp4", 12, 13, false, true),
    videoio_encapsulate_params_t("video/sample_322x242_15frames.yuv420p.libxvid.mp4", "mp4", 3, 4, false, true),
    videoio_encapsulate_params_t("video/sample_322x242_15frames.yuv420p.mpeg2video.mp4", "mpg", 12, 13, false, true),
    videoio_encapsulate_params_t("video/sample_322x242_15frames.yuv420p.mjpeg.mp4", "mp4", 0, 5, false, true),
    videoio_encapsulate_params_t("video/sample_322x242_15frames.yuv420p.libx264.mp4", "ts", 15, 15, true, true),
    videoio_encapsulate_params_t("../cv/tracking/faceocc2/data/faceocc2.webm", "webm", 128, 129, false, true),
    videoio_encapsulate_params_t("../cv/video/1920x1080.avi", "mp4", 12, 13, false, true),
    videoio_encapsulate_params_t("../cv/video/768x576.avi", "avi", 15, 16, false, true),
    // Not supported by with FFmpeg:
    //videoio_encapsulate_params_t("video/sample_322x242_15frames.yuv420p.libx265.mp4", "mp4", 15, 15, true, true),
    //videoio_encapsulate_params_t("video/sample_322x242_15frames.yuv420p.libvpx-vp9.mp4", "mp4", 15, 15, false, true),
};

INSTANTIATE_TEST_CASE_P(/**/, videoio_encapsulate, testing::ValuesIn(videoio_encapsulate_params));

TEST(videoio_encapsulate_set_idr, write)
{
    const VideoCaptureAPIs api = CAP_FFMPEG;
    if (!videoio_registry::hasBackend(api))
        throw SkipTestException("FFmpeg backend was not found");

    const string fileName = findDataFile("video/big_buck_bunny.mp4");
    const string ext = "mp4";
    const string fileNameOut = tempfile(cv::format("test_encapsulated_stream_set_idr.%s", ext.c_str()).c_str());

    // Use VideoWriter to encapsulate encoded video read with VideoReader
    {
        VideoCapture capRaw(fileName, api, { CAP_PROP_FORMAT, -1 });
        ASSERT_TRUE(capRaw.isOpened());
        const int width = static_cast<int>(capRaw.get(CAP_PROP_FRAME_WIDTH));
        const int height = static_cast<int>(capRaw.get(CAP_PROP_FRAME_HEIGHT));
        const double fps = capRaw.get(CAP_PROP_FPS);
        const int codecExtradataIndex = static_cast<int>(capRaw.get(CAP_PROP_CODEC_EXTRADATA_INDEX));
        Mat extraData;
        capRaw.retrieve(extraData, codecExtradataIndex);
        const int fourcc = static_cast<int>(capRaw.get(CAP_PROP_FOURCC));
        const bool mpeg4 = (fourcc == fourccFromString("FMP4"));

        VideoWriter container(fileNameOut, api, fourcc, fps, { width, height }, { VideoWriterProperties::VIDEOWRITER_PROP_RAW_VIDEO, 1 });
        ASSERT_TRUE(container.isOpened());
        Mat rawFrame;
        int i = 0;
        while (capRaw.read(rawFrame)) {
            ASSERT_FALSE(rawFrame.empty());
            if (i == 0 && mpeg4) {
                Mat tmp = rawFrame.clone();
                const size_t newSzt = tmp.total() + extraData.total();
                const int newSz = static_cast<int>(newSzt);
                ASSERT_TRUE(newSzt == static_cast<size_t>(newSz));
                rawFrame = Mat(1, newSz, CV_8UC1);
                memcpy(rawFrame.data, extraData.data, extraData.total());
                memcpy(rawFrame.data + extraData.total(), tmp.data, tmp.total());
            }
            if (capRaw.get(CAP_PROP_LRF_HAS_KEY_FRAME) != 0)
                container.set(VideoWriterProperties::VIDEOWRITER_PROP_KEY_FLAG, 1);
            else
                container.set(VideoWriterProperties::VIDEOWRITER_PROP_KEY_FLAG, 0);
            container.write(rawFrame);
            i++;
        }
        container.release();
    }

    std::cout << "Checking encapsulated video container: " << fileNameOut << std::endl;

    // Check encapsulated video container is "identical" to the original
    {
        VideoCapture capReference(fileName), capReferenceRaw(fileName, api, { CAP_PROP_FORMAT, -1 }), capActual(fileNameOut), capActualRaw(fileNameOut, api, { CAP_PROP_FORMAT, -1 });
        ASSERT_TRUE(capReference.isOpened());
        ASSERT_TRUE(capActual.isOpened());
        ASSERT_TRUE(capReferenceRaw.isOpened());
        ASSERT_TRUE(capActualRaw.isOpened());
        const double fpsReference = capReference.get(CAP_PROP_FPS);
        const double fpsActual = capActual.get(CAP_PROP_FPS);
        ASSERT_EQ(fpsReference, fpsActual);
        const int nFramesReference = static_cast<int>(capReference.get(CAP_PROP_FRAME_COUNT));
        const int nFramesActual = static_cast<int>(capActual.get(CAP_PROP_FRAME_COUNT));
        ASSERT_EQ(nFramesReference, nFramesActual);

        Mat reference, actual;
        for (int i = 0; i < nFramesReference; i++) {
            ASSERT_TRUE(capReference.read(reference));
            ASSERT_FALSE(reference.empty());
            ASSERT_TRUE(capActual.read(actual));
            ASSERT_FALSE(actual.empty());
            ASSERT_EQ(0, cvtest::norm(reference, actual, NORM_INF));
            ASSERT_TRUE(capReferenceRaw.grab());
            ASSERT_TRUE(capActualRaw.grab());
            const bool keyFrameReference = capActualRaw.get(CAP_PROP_LRF_HAS_KEY_FRAME) == 1.;
            const bool keyFrameActual = capActualRaw.get(CAP_PROP_LRF_HAS_KEY_FRAME) == 1.;
            ASSERT_EQ(keyFrameReference, keyFrameActual);
        }
    }

    ASSERT_EQ(0, remove(fileNameOut.c_str()));
}

>>>>>>> dd08328228f008f270a199b7fb25aab37a91135d
typedef tuple<string, string, int> videoio_skip_params_t;
typedef testing::TestWithParam< videoio_skip_params_t > videoio_skip;

TEST_P(videoio_skip, DISABLED_read)  // optional test, may fail in some configurations
{
#if CV_VERSION_MAJOR >= 4
    if (!videoio_registry::hasBackend(CAP_FFMPEG))
        throw SkipTestException("Backend was not found");
#endif

    const string path = get<0>(GetParam());
    const string env = get<1>(GetParam());
    const int expectedFrameNumber = get<2>(GetParam());

    #ifdef _WIN32
        _putenv_s("OPENCV_FFMPEG_CAPTURE_OPTIONS", env.c_str());
    #else
        setenv("OPENCV_FFMPEG_CAPTURE_OPTIONS", env.c_str(), 1);
    #endif
    VideoCapture container(findDataFile(path), CAP_FFMPEG);
    #ifdef _WIN32
        _putenv_s("OPENCV_FFMPEG_CAPTURE_OPTIONS", "");
    #else
        setenv("OPENCV_FFMPEG_CAPTURE_OPTIONS", "", 1);
    #endif

    ASSERT_TRUE(container.isOpened());

    Mat reference;
    int nframes = 0, n_err = 0;
    while (container.isOpened())
    {
        if (container.read(reference))
            nframes++;
        else if (++n_err > 3)
            break;
    }
    EXPECT_EQ(expectedFrameNumber, nframes);
}

const videoio_skip_params_t videoio_skip_params[] =
{
    videoio_skip_params_t("video/big_buck_bunny.mp4", "", 125),
    videoio_skip_params_t("video/big_buck_bunny.mp4", "avdiscard;nonkey", 11)
};

INSTANTIATE_TEST_CASE_P(/**/, videoio_skip, testing::ValuesIn(videoio_skip_params));

//==========================================================================

static void generateFrame(Mat &frame, unsigned int i, const Point &center, const Scalar &color)
{
    frame = Scalar::all(i % 255);
    stringstream buf(ios::out);
    buf << "frame #" << i;
    putText(frame, buf.str(), Point(50, center.y), FONT_HERSHEY_SIMPLEX, 5.0, color, 5, CV_AA);
    circle(frame, center, i + 2, color, 2, CV_AA);
}

TEST(videoio_ffmpeg, parallel)
{
    if (!videoio_registry::hasBackend(CAP_FFMPEG))
        throw SkipTestException("FFmpeg backend was not found");

    const int NUM = 4;
    const int GRAN = 4;
    const Range R(0, NUM);
    const Size sz(1020, 900);
    const int frameNum = 300;
    const Scalar color(Scalar::all(0));
    const Point center(sz.height / 2, sz.width / 2);

    // Generate filenames
    vector<string> files;
    for (int i = 0; i < NUM; ++i)
    {
        ostringstream stream;
        stream << i << ".avi";
        files.push_back(tempfile(stream.str().c_str()));
    }
    // Write videos
    {
        vector< Ptr<VideoWriter> > writers(NUM);
        auto makeWriters = [&](const Range &r)
        {
            for (int i = r.start; i != r.end; ++i)
                writers[i] = makePtr<VideoWriter>(files[i],
                                                  CAP_FFMPEG,
                                                  VideoWriter::fourcc('X','V','I','D'),
                                                  25.0f,
                                                  sz);
        };
        parallel_for_(R, makeWriters, GRAN);
        for(int i = 0; i < NUM; ++i)
        {
            ASSERT_TRUE(writers[i]);
            ASSERT_TRUE(writers[i]->isOpened());
        }
        auto writeFrames = [&](const Range &r)
        {
            for (int j = r.start; j < r.end; ++j)
            {
                Mat frame(sz, CV_8UC3);
                for (int i = 0; i < frameNum; ++i)
                {
                    generateFrame(frame, i, center, color);
                    writers[j]->write(frame);
                }
            }
        };
        parallel_for_(R, writeFrames, GRAN);
    }
    // Read videos
    {
        vector< Ptr<VideoCapture> > readers(NUM);
        auto makeCaptures = [&](const Range &r)
        {
            for (int i = r.start; i != r.end; ++i)
                readers[i] = makePtr<VideoCapture>(files[i], CAP_FFMPEG);
        };
        parallel_for_(R, makeCaptures, GRAN);
        for(int i = 0; i < NUM; ++i)
        {
            ASSERT_TRUE(readers[i]);
            ASSERT_TRUE(readers[i]->isOpened());
        }
        auto readFrames = [&](const Range &r)
        {
            for (int j = r.start; j < r.end; ++j)
            {
                Mat reference(sz, CV_8UC3);
                for (int i = 0; i < frameNum; ++i)
                {
                    Mat actual;
                    EXPECT_TRUE(readers[j]->read(actual));
                    EXPECT_FALSE(actual.empty());
                    generateFrame(reference, i, center, color);
                    EXPECT_EQ(reference.size(), actual.size());
                    EXPECT_EQ(reference.depth(), actual.depth());
                    EXPECT_EQ(reference.channels(), actual.channels());
                    EXPECT_GE(cvtest::PSNR(actual, reference), 35.0) << "cap" << j << ", frame " << i;
                }
            }
        };
        parallel_for_(R, readFrames, GRAN);
    }
    // Remove files
    for(int i = 0; i < NUM; ++i)
    {
        remove(files[i].c_str());
    }
}

typedef std::pair<VideoCaptureProperties, double> cap_property_t;
typedef std::vector<cap_property_t> cap_properties_t;
typedef std::pair<std::string, cap_properties_t> ffmpeg_cap_properties_param_t;
typedef testing::TestWithParam<ffmpeg_cap_properties_param_t> ffmpeg_cap_properties;

#ifdef _WIN32
namespace {
::testing::AssertionResult IsOneOf(double value, double expected1, double expected2)
{
    // internal floating point class is used to perform accurate floating point types comparison
    typedef ::testing::internal::FloatingPoint<double> FloatingPoint;

    FloatingPoint val(value);
    if (val.AlmostEquals(FloatingPoint(expected1)) || val.AlmostEquals(FloatingPoint(expected2)))
    {
        return ::testing::AssertionSuccess();
    }
    else
    {
        return ::testing::AssertionFailure()
               << value << " is neither  equal to " << expected1 << " nor " << expected2;
    }
}
}
#endif

TEST_P(ffmpeg_cap_properties, can_read_property)
{
    if (!videoio_registry::hasBackend(CAP_FFMPEG))
        throw SkipTestException("FFmpeg backend was not found");

    ffmpeg_cap_properties_param_t parameters = GetParam();
    const std::string path = parameters.first;
    const cap_properties_t properties = parameters.second;

    VideoCapture cap(findDataFile(path), CAP_FFMPEG);
    ASSERT_TRUE(cap.isOpened()) << "Can not open " << findDataFile(path);

    for (std::size_t i = 0; i < properties.size(); ++i)
    {
        const cap_property_t& prop = properties[i];
        const double actualValue = cap.get(static_cast<int>(prop.first));
    #ifndef _WIN32
        EXPECT_DOUBLE_EQ(actualValue, prop.second)
            << "Property " << static_cast<int>(prop.first) << " has wrong value";
    #else
        EXPECT_TRUE(IsOneOf(actualValue, prop.second, 0.0))
            << "Property " << static_cast<int>(prop.first) << " has wrong value";
    #endif
    }
}

cap_properties_t loadBigBuckBunnyFFProbeResults() {
    cap_property_t properties[] = { cap_property_t(CAP_PROP_BITRATE, 5851.),
                                    cap_property_t(CAP_PROP_FPS, 24.),
                                    cap_property_t(CAP_PROP_FRAME_HEIGHT, 384.),
                                    cap_property_t(CAP_PROP_FRAME_WIDTH, 672.) };
    return cap_properties_t(properties, properties + sizeof(properties) / sizeof(cap_property_t));
}

const ffmpeg_cap_properties_param_t videoio_ffmpeg_properties[] = {
    ffmpeg_cap_properties_param_t("video/big_buck_bunny.avi", loadBigBuckBunnyFFProbeResults())
};

INSTANTIATE_TEST_CASE_P(videoio, ffmpeg_cap_properties, testing::ValuesIn(videoio_ffmpeg_properties));



// related issue: https://github.com/opencv/opencv/issues/15499
TEST(videoio, mp4_orientation_meta_auto)
{
    if (!videoio_registry::hasBackend(CAP_FFMPEG))
        throw SkipTestException("FFmpeg backend was not found");

    string video_file = string(cvtest::TS::ptr()->get_data_path()) + "video/big_buck_bunny_rotated.mp4";

    VideoCapture cap;
    EXPECT_NO_THROW(cap.open(video_file, CAP_FFMPEG));
    ASSERT_TRUE(cap.isOpened()) << "Can't open the video: " << video_file << " with backend " << CAP_FFMPEG << std::endl;

    cap.set(CAP_PROP_ORIENTATION_AUTO, true);
    if (cap.get(CAP_PROP_ORIENTATION_AUTO) == 0)
        throw SkipTestException("FFmpeg frame rotation metadata is not supported");

    Size actual;
    EXPECT_NO_THROW(actual = Size((int)cap.get(CAP_PROP_FRAME_WIDTH),
                                    (int)cap.get(CAP_PROP_FRAME_HEIGHT)));
    EXPECT_EQ(384, actual.width);
    EXPECT_EQ(672, actual.height);

    Mat frame;

    cap >> frame;

    ASSERT_EQ(384, frame.cols);
    ASSERT_EQ(672, frame.rows);
}

// related issue: https://github.com/opencv/opencv/issues/15499
TEST(videoio, mp4_orientation_no_rotation)
{
    if (!videoio_registry::hasBackend(CAP_FFMPEG))
        throw SkipTestException("FFmpeg backend was not found");

    string video_file = string(cvtest::TS::ptr()->get_data_path()) + "video/big_buck_bunny_rotated.mp4";

    VideoCapture cap;
    EXPECT_NO_THROW(cap.open(video_file, CAP_FFMPEG));
    cap.set(CAP_PROP_ORIENTATION_AUTO, 0);
    ASSERT_TRUE(cap.isOpened()) << "Can't open the video: " << video_file << " with backend " << CAP_FFMPEG << std::endl;
    ASSERT_FALSE(cap.get(CAP_PROP_ORIENTATION_AUTO));

    Size actual;
    EXPECT_NO_THROW(actual = Size((int)cap.get(CAP_PROP_FRAME_WIDTH),
                                    (int)cap.get(CAP_PROP_FRAME_HEIGHT)));
    EXPECT_EQ(672, actual.width);
    EXPECT_EQ(384, actual.height);

    Mat frame;

    cap >> frame;

    ASSERT_EQ(672, frame.cols);
    ASSERT_EQ(384, frame.rows);
}


static void ffmpeg_check_read_raw(VideoCapture& cap)
{
    ASSERT_TRUE(cap.isOpened()) << "Can't open the video";

    Mat data;
    cap >> data;
    EXPECT_EQ(CV_8UC1, data.type()) << "CV_8UC1 != " << typeToString(data.type());
    EXPECT_TRUE(data.rows == 1 || data.cols == 1) << data.size;
    EXPECT_EQ((size_t)29729, data.total());

    cap >> data;
    EXPECT_EQ(CV_8UC1, data.type()) << "CV_8UC1 != " << typeToString(data.type());
    EXPECT_TRUE(data.rows == 1 || data.cols == 1) << data.size;
    EXPECT_EQ((size_t)37118, data.total());
<<<<<<< HEAD
=======

    // 12 is the nearset key frame to frame 18
    EXPECT_TRUE(cap.set(CAP_PROP_POS_FRAMES, 18.));
    EXPECT_EQ(cap.get(CAP_PROP_POS_FRAMES), 12.);
    cap >> data;
    EXPECT_EQ(CV_8UC1, data.type()) << "CV_8UC1 != " << typeToString(data.type());
    EXPECT_TRUE(data.rows == 1 || data.cols == 1) << data.size;
    EXPECT_EQ((size_t)8726, data.total());
}

TEST(videoio_ffmpeg, ffmpeg_check_extra_data)
{
    if (!videoio_registry::hasBackend(CAP_FFMPEG))
        throw SkipTestException("FFmpeg backend was not found");

    string video_file = findDataFile("video/big_buck_bunny.mp4");
    VideoCapture cap;
    EXPECT_NO_THROW(cap.open(video_file, CAP_FFMPEG));
    ASSERT_TRUE(cap.isOpened()) << "Can't open the video";
    const int codecExtradataIdx = (int)cap.get(CAP_PROP_CODEC_EXTRADATA_INDEX);
    Mat data;
    ASSERT_TRUE(cap.retrieve(data, codecExtradataIdx));
    EXPECT_EQ(CV_8UC1, data.type()) << "CV_8UC1 != " << typeToString(data.type());
    EXPECT_TRUE(data.rows == 1 || data.cols == 1) << data.size;
    EXPECT_EQ((size_t)45, data.total());
>>>>>>> dd08328228f008f270a199b7fb25aab37a91135d
}

TEST(videoio_ffmpeg, open_with_property)
{
    if (!videoio_registry::hasBackend(CAP_FFMPEG))
        throw SkipTestException("FFmpeg backend was not found");

    string video_file = findDataFile("video/big_buck_bunny.mp4");
    VideoCapture cap;
    EXPECT_NO_THROW(cap.open(video_file, CAP_FFMPEG, {
        CAP_PROP_FORMAT, -1  // demux only
    }));

<<<<<<< HEAD
=======
    // confirm properties are returned without initializing AVCodecContext
    EXPECT_EQ(cap.get(CAP_PROP_FORMAT), -1);
    EXPECT_EQ(static_cast<int>(cap.get(CAP_PROP_FOURCC)), fourccFromString("FMP4"));
    EXPECT_EQ(cap.get(CAP_PROP_N_THREADS), 0.0);
    EXPECT_EQ(cap.get(CAP_PROP_FRAME_HEIGHT), 384.0);
    EXPECT_EQ(cap.get(CAP_PROP_FRAME_WIDTH), 672.0);
    EXPECT_EQ(cap.get(CAP_PROP_FRAME_COUNT), 125);
    EXPECT_EQ(cap.get(CAP_PROP_FPS), 24.0);
>>>>>>> dd08328228f008f270a199b7fb25aab37a91135d
    ffmpeg_check_read_raw(cap);
}

TEST(videoio_ffmpeg, create_with_property)
{
    if (!videoio_registry::hasBackend(CAP_FFMPEG))
        throw SkipTestException("FFmpeg backend was not found");

    string video_file = findDataFile("video/big_buck_bunny.mp4");
    VideoCapture cap(video_file, CAP_FFMPEG, {
        CAP_PROP_FORMAT, -1  // demux only
    });

<<<<<<< HEAD
=======
    // confirm properties are returned without initializing AVCodecContext
    EXPECT_TRUE(cap.get(CAP_PROP_FORMAT) == -1);
    EXPECT_EQ(static_cast<int>(cap.get(CAP_PROP_FOURCC)), fourccFromString("FMP4"));
    EXPECT_EQ(cap.get(CAP_PROP_N_THREADS), 0.0);
    EXPECT_EQ(cap.get(CAP_PROP_FRAME_HEIGHT), 384.0);
    EXPECT_EQ(cap.get(CAP_PROP_FRAME_WIDTH), 672.0);
    EXPECT_EQ(cap.get(CAP_PROP_FRAME_COUNT), 125);
    EXPECT_EQ(cap.get(CAP_PROP_FPS), 24.0);
>>>>>>> dd08328228f008f270a199b7fb25aab37a91135d
    ffmpeg_check_read_raw(cap);
}

TEST(videoio_ffmpeg, create_with_property_badarg)
{
    if (!videoio_registry::hasBackend(CAP_FFMPEG))
        throw SkipTestException("FFmpeg backend was not found");

    string video_file = findDataFile("video/big_buck_bunny.mp4");
    VideoCapture cap(video_file, CAP_FFMPEG, {
        CAP_PROP_FORMAT, -2  // invalid
    });
    EXPECT_FALSE(cap.isOpened());
}


}} // namespace
