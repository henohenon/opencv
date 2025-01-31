/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                           License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2013, OpenCV Foundation, all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

/*M///////////////////////////////////////////////////////////////////////////////////////
// Author: Sajjad Taheri, University of California, Irvine. sajjadt[at]uci[dot]edu
//
//                             LICENSE AGREEMENT
// Copyright (c) 2015 The Regents of the University of California (Regents)
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. Neither the name of the University nor the
//    names of its contributors may be used to endorse or promote products
//    derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ''AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL CONTRIBUTORS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//M*/

#include <emscripten/bind.h>

#include "opencv2/core.hpp"
#include "opencv2/core/async.hpp"
#include "opencv2/core/base.hpp"
#include "opencv2/core/bindings_utils.hpp"
#include "opencv2/core/check.hpp"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/optim.hpp"
#include "opencv2/core/ovx.hpp"
#include "opencv2/core/parallel/parallel_backend.hpp"
#include "opencv2/core/persistence.hpp"
#include "opencv2/core/quaternion.hpp"
#include "opencv2/core/types.hpp"
#include "opencv2/core/utility.hpp"
#include "opencv2/core/utils/logger.defines.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/imgproc/bindings.hpp"
#include "opencv2/imgproc/segmentation.hpp"
#include "opencv2/photo.hpp"
#include "opencv2/photo/cuda.hpp"
#include "opencv2/dnn/dict.hpp"
#include "opencv2/dnn/dnn.hpp"
#include "opencv2/dnn/utils/inference_engine.hpp"
#include "opencv2/features2d.hpp"
#include "opencv2/calib3d.hpp"
#include "opencv2/objdetect.hpp"
#include "opencv2/objdetect/aruco_board.hpp"
#include "opencv2/objdetect/aruco_detector.hpp"
#include "opencv2/objdetect/aruco_dictionary.hpp"
#include "opencv2/objdetect/barcode.hpp"
#include "opencv2/objdetect/charuco_detector.hpp"
#include "opencv2/objdetect/face.hpp"
#include "opencv2/objdetect/graphical_code_detector.hpp"
#include "opencv2/video/background_segm.hpp"
#include "opencv2/video/detail/tracking.detail.hpp"
#include "opencv2/video/tracking.hpp"
#include "../../../modules/core/src/parallel_impl.hpp"

#ifdef TEST_WASM_INTRIN
#include "../../../modules/core/include/opencv2/core/hal/intrin.hpp"
#include "../../../modules/core/include/opencv2/core/utils/trace.hpp"
#include "../../../modules/ts/include/opencv2/ts/ts_gtest.h"
namespace cv {
namespace hal {
#include "../../../modules/core/test/test_intrin_utils.hpp"
}
}
#endif

using namespace emscripten;
using namespace cv;

using namespace cv::segmentation;  // FIXIT

#ifdef HAVE_OPENCV_OBJDETECT
using namespace cv::aruco;
typedef aruco::DetectorParameters aruco_DetectorParameters;
typedef QRCodeDetectorAruco::Params QRCodeDetectorAruco_Params;
#endif

#ifdef HAVE_OPENCV_DNN
using namespace cv::dnn;
#endif

#ifdef HAVE_OPENCV_FEATURES2D
typedef SimpleBlobDetector::Params SimpleBlobDetector_Params;
#endif

#ifdef HAVE_OPENCV_VIDEO
typedef TrackerMIL::Params TrackerMIL_Params;
#endif

// HACK: JS generator ommits namespace for parameter types for some reason. Added typedef to handle std::string correctly
typedef std::string string;

namespace binding_utils
{
    template<typename classT, typename enumT>
    static inline typename std::underlying_type<enumT>::type classT::* underlying_ptr(enumT classT::* enum_ptr)
    {
        return reinterpret_cast<typename std::underlying_type<enumT>::type classT::*>(enum_ptr);
    }

    template<typename T>
    emscripten::val matData(const cv::Mat& mat)
    {
        return emscripten::val(emscripten::memory_view<T>((mat.total()*mat.elemSize())/sizeof(T),
                               (T*)mat.data));
    }

    template<typename T>
    emscripten::val matPtr(const cv::Mat& mat, int i)
    {
        return emscripten::val(emscripten::memory_view<T>(mat.step1(0), mat.ptr<T>(i)));
    }

    template<typename T>
    emscripten::val matPtr(const cv::Mat& mat, int i, int j)
    {
        return emscripten::val(emscripten::memory_view<T>(mat.step1(1), mat.ptr<T>(i,j)));
    }

    cv::Mat* createMat(int rows, int cols, int type, intptr_t data, size_t step)
    {
        return new cv::Mat(rows, cols, type, reinterpret_cast<void*>(data), step);
    }

    static emscripten::val getMatSize(const cv::Mat& mat)
    {
        emscripten::val size = emscripten::val::array();
        for (int i = 0; i < mat.dims; i++) {
            size.call<void>("push", mat.size[i]);
        }
        return size;
    }

    static emscripten::val getMatStep(const cv::Mat& mat)
    {
        emscripten::val step = emscripten::val::array();
        for (int i = 0; i < mat.dims; i++) {
            step.call<void>("push", mat.step[i]);
        }
        return step;
    }

    static Mat matEye(int rows, int cols, int type)
    {
        return Mat(cv::Mat::eye(rows, cols, type));
    }

    static Mat matEye(Size size, int type)
    {
        return Mat(cv::Mat::eye(size, type));
    }

    void convertTo(const Mat& obj, Mat& m, int rtype, double alpha, double beta)
    {
        obj.convertTo(m, rtype, alpha, beta);
    }

    void convertTo(const Mat& obj, Mat& m, int rtype)
    {
        obj.convertTo(m, rtype);
    }

    void convertTo(const Mat& obj, Mat& m, int rtype, double alpha)
    {
        obj.convertTo(m, rtype, alpha);
    }

    Size matSize(const cv::Mat& mat)
    {
        return mat.size();
    }

    cv::Mat matZeros(int arg0, int arg1, int arg2)
    {
        return cv::Mat::zeros(arg0, arg1, arg2);
    }

    cv::Mat matZeros(cv::Size arg0, int arg1)
    {
        return cv::Mat::zeros(arg0,arg1);
    }

    cv::Mat matOnes(int arg0, int arg1, int arg2)
    {
        return cv::Mat::ones(arg0, arg1, arg2);
    }

    cv::Mat matOnes(cv::Size arg0, int arg1)
    {
        return cv::Mat::ones(arg0, arg1);
    }

    double matDot(const cv::Mat& obj, const Mat& mat)
    {
        return  obj.dot(mat);
    }

    Mat matMul(const cv::Mat& obj, const Mat& mat, double scale)
    {
        return  Mat(obj.mul(mat, scale));
    }

    Mat matT(const cv::Mat& obj)
    {
        return  Mat(obj.t());
    }

    Mat matInv(const cv::Mat& obj, int type)
    {
        return  Mat(obj.inv(type));
    }

    void matCopyTo(const cv::Mat& obj, cv::Mat& mat)
    {
        return obj.copyTo(mat);
    }

    void matCopyTo(const cv::Mat& obj, cv::Mat& mat, const cv::Mat& mask)
    {
        return obj.copyTo(mat, mask);
    }

    Mat matDiag(const cv::Mat& obj, int d)
    {
        return obj.diag(d);
    }

    Mat matDiag(const cv::Mat& obj)
    {
        return obj.diag();
    }

    void matSetTo(cv::Mat& obj, const cv::Scalar& s)
    {
        obj.setTo(s);
    }

    void matSetTo(cv::Mat& obj, const cv::Scalar& s, const cv::Mat& mask)
    {
        obj.setTo(s, mask);
    }

    emscripten::val rotatedRectPoints(const cv::RotatedRect& obj)
    {
        cv::Point2f points[4];
        obj.points(points);
        emscripten::val pointsArray = emscripten::val::array();
        for (int i = 0; i < 4; i++) {
            pointsArray.call<void>("push", points[i]);
        }
        return pointsArray;
    }

    Rect rotatedRectBoundingRect(const cv::RotatedRect& obj)
    {
        return obj.boundingRect();
    }

    Rect2f rotatedRectBoundingRect2f(const cv::RotatedRect& obj)
    {
        return obj.boundingRect2f();
    }

    int cvMatDepth(int flags)
    {
        return CV_MAT_DEPTH(flags);
    }

    class MinMaxLoc
    {
    public:
        double minVal;
        double maxVal;
        Point minLoc;
        Point maxLoc;
    };

    MinMaxLoc minMaxLoc(const cv::Mat& src, const cv::Mat& mask)
    {
        MinMaxLoc result;
        cv::minMaxLoc(src, &result.minVal, &result.maxVal, &result.minLoc, &result.maxLoc, mask);
        return result;
    }

    MinMaxLoc minMaxLoc_1(const cv::Mat& src)
    {
        MinMaxLoc result;
        cv::minMaxLoc(src, &result.minVal, &result.maxVal, &result.minLoc, &result.maxLoc);
        return result;
    }

    class Circle
    {
    public:
        Point2f center;
        float radius;
    };

#ifdef HAVE_OPENCV_IMGPROC
    Circle minEnclosingCircle(const cv::Mat& points)
    {
        Circle circle;
        cv::minEnclosingCircle(points, circle.center, circle.radius);
        return circle;
    }

    int floodFill_withRect_helper(cv::Mat& arg1, cv::Mat& arg2, Point arg3, Scalar arg4, emscripten::val arg5, Scalar arg6 = Scalar(), Scalar arg7 = Scalar(), int arg8 = 4)
    {
        cv::Rect rect;

        int rc = cv::floodFill(arg1, arg2, arg3, arg4, &rect, arg6, arg7, arg8);

        arg5.set("x", emscripten::val(rect.x));
        arg5.set("y", emscripten::val(rect.y));
        arg5.set("width", emscripten::val(rect.width));
        arg5.set("height", emscripten::val(rect.height));

        return rc;
    }

    int floodFill_wrapper(cv::Mat& arg1, cv::Mat& arg2, Point arg3, Scalar arg4, emscripten::val arg5, Scalar arg6, Scalar arg7, int arg8) {
        return floodFill_withRect_helper(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
    }

    int floodFill_wrapper_1(cv::Mat& arg1, cv::Mat& arg2, Point arg3, Scalar arg4, emscripten::val arg5, Scalar arg6, Scalar arg7) {
        return floodFill_withRect_helper(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
    }

    int floodFill_wrapper_2(cv::Mat& arg1, cv::Mat& arg2, Point arg3, Scalar arg4, emscripten::val arg5, Scalar arg6) {
        return floodFill_withRect_helper(arg1, arg2, arg3, arg4, arg5, arg6);
    }

    int floodFill_wrapper_3(cv::Mat& arg1, cv::Mat& arg2, Point arg3, Scalar arg4, emscripten::val arg5) {
        return floodFill_withRect_helper(arg1, arg2, arg3, arg4, arg5);
    }

    int floodFill_wrapper_4(cv::Mat& arg1, cv::Mat& arg2, Point arg3, Scalar arg4) {
        return cv::floodFill(arg1, arg2, arg3, arg4);
    }
#endif

#ifdef HAVE_OPENCV_VIDEO
    emscripten::val CamShiftWrapper(const cv::Mat& arg1, Rect& arg2, TermCriteria arg3)
    {
        RotatedRect rotatedRect = cv::CamShift(arg1, arg2, arg3);
        emscripten::val result = emscripten::val::array();
        result.call<void>("push", rotatedRect);
        result.call<void>("push", arg2);
        return result;
    }

    emscripten::val meanShiftWrapper(const cv::Mat& arg1, Rect& arg2, TermCriteria arg3)
    {
        int n = cv::meanShift(arg1, arg2, arg3);
        emscripten::val result = emscripten::val::array();
        result.call<void>("push", n);
        result.call<void>("push", arg2);
        return result;
    }


    void Tracker_init_wrapper(cv::Tracker& arg0, const cv::Mat& arg1, const Rect& arg2)
    {
        return arg0.init(arg1, arg2);
    }

    emscripten::val Tracker_update_wrapper(cv::Tracker& arg0, const cv::Mat& arg1)
    {
        Rect rect;
        bool update = arg0.update(arg1, rect);

        emscripten::val result = emscripten::val::array();
        result.call<void>("push", update);
        result.call<void>("push", rect);
        return result;
    }
#endif  // HAVE_OPENCV_VIDEO

    std::string getExceptionMsg(const cv::Exception& e) {
        return e.msg;
    }

    void setExceptionMsg(cv::Exception& e, std::string msg) {
        e.msg = msg;
        return;
    }

    cv::Exception exceptionFromPtr(intptr_t ptr) {
        return *reinterpret_cast<cv::Exception*>(ptr);
    }

    std::string getBuildInformation() {
        return cv::getBuildInformation();
    }

#ifdef TEST_WASM_INTRIN
    void test_hal_intrin_uint8() {
        cv::hal::test_hal_intrin_uint8();
    }
    void test_hal_intrin_int8() {
        cv::hal::test_hal_intrin_int8();
    }
    void test_hal_intrin_uint16() {
        cv::hal::test_hal_intrin_uint16();
    }
    void test_hal_intrin_int16() {
        cv::hal::test_hal_intrin_int16();
    }
    void test_hal_intrin_uint32() {
        cv::hal::test_hal_intrin_uint32();
    }
    void test_hal_intrin_int32() {
        cv::hal::test_hal_intrin_int32();
    }
    void test_hal_intrin_uint64() {
        cv::hal::test_hal_intrin_uint64();
    }
    void test_hal_intrin_int64() {
        cv::hal::test_hal_intrin_int64();
    }
    void test_hal_intrin_float32() {
        cv::hal::test_hal_intrin_float32();
    }
    void test_hal_intrin_float64() {
        cv::hal::test_hal_intrin_float64();
    }
    void test_hal_intrin_all() {
        cv::hal::test_hal_intrin_uint8();
        cv::hal::test_hal_intrin_int8();
        cv::hal::test_hal_intrin_uint16();
        cv::hal::test_hal_intrin_int16();
        cv::hal::test_hal_intrin_uint32();
        cv::hal::test_hal_intrin_int32();
        cv::hal::test_hal_intrin_uint64();
        cv::hal::test_hal_intrin_int64();
        cv::hal::test_hal_intrin_float32();
        cv::hal::test_hal_intrin_float64();
    }
#endif
}

EMSCRIPTEN_BINDINGS(binding_utils)
{
    register_vector<int>("IntVector");
    register_vector<char>("CharVector");
    register_vector<float>("FloatVector");
    register_vector<double>("DoubleVector");
    register_vector<std::string>("StringVector");
    register_vector<cv::Point>("PointVector");
    register_vector<cv::Mat>("MatVector");
    register_vector<cv::Rect>("RectVector");
    register_vector<cv::KeyPoint>("KeyPointVector");
    register_vector<cv::DMatch>("DMatchVector");
    register_vector<std::vector<cv::DMatch>>("DMatchVectorVector");


    emscripten::class_<cv::Mat>("Mat")
        .constructor<>()
        .constructor<const Mat&>()
        .constructor<Size, int>()
        .constructor<int, int, int>()
        .constructor<int, int, int, const Scalar&>()
        .constructor(&binding_utils::createMat, allow_raw_pointers())

        .class_function("eye", select_overload<Mat(Size, int)>(&binding_utils::matEye))
        .class_function("eye", select_overload<Mat(int, int, int)>(&binding_utils::matEye))
        .class_function("ones", select_overload<Mat(Size, int)>(&binding_utils::matOnes))
        .class_function("ones", select_overload<Mat(int, int, int)>(&binding_utils::matOnes))
        .class_function("zeros", select_overload<Mat(Size, int)>(&binding_utils::matZeros))
        .class_function("zeros", select_overload<Mat(int, int, int)>(&binding_utils::matZeros))

        .property("rows", &cv::Mat::rows)
        .property("cols", &cv::Mat::cols)
        .property("matSize", &binding_utils::getMatSize)
        .property("step", &binding_utils::getMatStep)
        .property("data", &binding_utils::matData<unsigned char>)
        .property("data8S", &binding_utils::matData<char>)
        .property("data16U", &binding_utils::matData<unsigned short>)
        .property("data16S", &binding_utils::matData<short>)
        .property("data32S", &binding_utils::matData<int>)
        .property("data32F", &binding_utils::matData<float>)
        .property("data64F", &binding_utils::matData<double>)

        .function("elemSize", select_overload<size_t()const>(&cv::Mat::elemSize))
        .function("elemSize1", select_overload<size_t()const>(&cv::Mat::elemSize1))
        .function("channels", select_overload<int()const>(&cv::Mat::channels))
        .function("convertTo", select_overload<void(const Mat&, Mat&, int, double, double)>(&binding_utils::convertTo))
        .function("convertTo", select_overload<void(const Mat&, Mat&, int)>(&binding_utils::convertTo))
        .function("convertTo", select_overload<void(const Mat&, Mat&, int, double)>(&binding_utils::convertTo))
        .function("total", select_overload<size_t()const>(&cv::Mat::total))
        .function("row", select_overload<Mat(int)const>(&cv::Mat::row))
        .function("create", select_overload<void(int, int, int)>(&cv::Mat::create))
        .function("create", select_overload<void(Size, int)>(&cv::Mat::create))
        .function("rowRange", select_overload<Mat(int, int)const>(&cv::Mat::rowRange))
        .function("rowRange", select_overload<Mat(const Range&)const>(&cv::Mat::rowRange))
        .function("copyTo", select_overload<void(const Mat&, Mat&)>(&binding_utils::matCopyTo))
        .function("copyTo", select_overload<void(const Mat&, Mat&, const Mat&)>(&binding_utils::matCopyTo))
        .function("type", select_overload<int()const>(&cv::Mat::type))
        .function("empty", select_overload<bool()const>(&cv::Mat::empty))
        .function("colRange", select_overload<Mat(int, int)const>(&cv::Mat::colRange))
        .function("colRange", select_overload<Mat(const Range&)const>(&cv::Mat::colRange))
        .function("step1", select_overload<size_t(int)const>(&cv::Mat::step1))
        .function("clone", select_overload<Mat()const>(&cv::Mat::clone))
        .function("depth", select_overload<int()const>(&cv::Mat::depth))
        .function("col", select_overload<Mat(int)const>(&cv::Mat::col))
        .function("dot", select_overload<double(const Mat&, const Mat&)>(&binding_utils::matDot))
        .function("mul", select_overload<Mat(const Mat&, const Mat&, double)>(&binding_utils::matMul))
        .function("inv", select_overload<Mat(const Mat&, int)>(&binding_utils::matInv))
        .function("t", select_overload<Mat(const Mat&)>(&binding_utils::matT))
        .function("roi", select_overload<Mat(const Rect&)const>(&cv::Mat::operator()))
        .function("diag", select_overload<Mat(const Mat&, int)>(&binding_utils::matDiag))
        .function("diag", select_overload<Mat(const Mat&)>(&binding_utils::matDiag))
        .function("isContinuous", select_overload<bool()const>(&cv::Mat::isContinuous))
        .function("setTo", select_overload<void(Mat&, const Scalar&)>(&binding_utils::matSetTo))
        .function("setTo", select_overload<void(Mat&, const Scalar&, const Mat&)>(&binding_utils::matSetTo))
        .function("size", select_overload<Size(const Mat&)>(&binding_utils::matSize))

        .function("ptr", select_overload<val(const Mat&, int)>(&binding_utils::matPtr<unsigned char>))
        .function("ptr", select_overload<val(const Mat&, int, int)>(&binding_utils::matPtr<unsigned char>))
        .function("ucharPtr", select_overload<val(const Mat&, int)>(&binding_utils::matPtr<unsigned char>))
        .function("ucharPtr", select_overload<val(const Mat&, int, int)>(&binding_utils::matPtr<unsigned char>))
        .function("charPtr", select_overload<val(const Mat&, int)>(&binding_utils::matPtr<char>))
        .function("charPtr", select_overload<val(const Mat&, int, int)>(&binding_utils::matPtr<char>))
        .function("shortPtr", select_overload<val(const Mat&, int)>(&binding_utils::matPtr<short>))
        .function("shortPtr", select_overload<val(const Mat&, int, int)>(&binding_utils::matPtr<short>))
        .function("ushortPtr", select_overload<val(const Mat&, int)>(&binding_utils::matPtr<unsigned short>))
        .function("ushortPtr", select_overload<val(const Mat&, int, int)>(&binding_utils::matPtr<unsigned short>))
        .function("intPtr", select_overload<val(const Mat&, int)>(&binding_utils::matPtr<int>))
        .function("intPtr", select_overload<val(const Mat&, int, int)>(&binding_utils::matPtr<int>))
        .function("floatPtr", select_overload<val(const Mat&, int)>(&binding_utils::matPtr<float>))
        .function("floatPtr", select_overload<val(const Mat&, int, int)>(&binding_utils::matPtr<float>))
        .function("doublePtr", select_overload<val(const Mat&, int)>(&binding_utils::matPtr<double>))
        .function("doublePtr", select_overload<val(const Mat&, int, int)>(&binding_utils::matPtr<double>))

        .function("charAt", select_overload<char&(int)>(&cv::Mat::at<char>))
        .function("charAt", select_overload<char&(int, int)>(&cv::Mat::at<char>))
        .function("charAt", select_overload<char&(int, int, int)>(&cv::Mat::at<char>))
        .function("ucharAt", select_overload<unsigned char&(int)>(&cv::Mat::at<unsigned char>))
        .function("ucharAt", select_overload<unsigned char&(int, int)>(&cv::Mat::at<unsigned char>))
        .function("ucharAt", select_overload<unsigned char&(int, int, int)>(&cv::Mat::at<unsigned char>))
        .function("shortAt", select_overload<short&(int)>(&cv::Mat::at<short>))
        .function("shortAt", select_overload<short&(int, int)>(&cv::Mat::at<short>))
        .function("shortAt", select_overload<short&(int, int, int)>(&cv::Mat::at<short>))
        .function("ushortAt", select_overload<unsigned short&(int)>(&cv::Mat::at<unsigned short>))
        .function("ushortAt", select_overload<unsigned short&(int, int)>(&cv::Mat::at<unsigned short>))
        .function("ushortAt", select_overload<unsigned short&(int, int, int)>(&cv::Mat::at<unsigned short>))
        .function("intAt", select_overload<int&(int)>(&cv::Mat::at<int>) )
        .function("intAt", select_overload<int&(int, int)>(&cv::Mat::at<int>) )
        .function("intAt", select_overload<int&(int, int, int)>(&cv::Mat::at<int>) )
        .function("floatAt", select_overload<float&(int)>(&cv::Mat::at<float>))
        .function("floatAt", select_overload<float&(int, int)>(&cv::Mat::at<float>))
        .function("floatAt", select_overload<float&(int, int, int)>(&cv::Mat::at<float>))
        .function("doubleAt", select_overload<double&(int, int, int)>(&cv::Mat::at<double>))
        .function("doubleAt", select_overload<double&(int)>(&cv::Mat::at<double>))
        .function("doubleAt", select_overload<double&(int, int)>(&cv::Mat::at<double>));

    emscripten::value_object<cv::Range>("Range")
        .field("start", &cv::Range::start)
        .field("end", &cv::Range::end);

    emscripten::value_object<cv::TermCriteria>("TermCriteria")
        .field("type", &cv::TermCriteria::type)
        .field("maxCount", &cv::TermCriteria::maxCount)
        .field("epsilon", &cv::TermCriteria::epsilon);

#define EMSCRIPTEN_CV_SIZE(type) \
    emscripten::value_object<type>("#type") \
        .field("width", &type::width) \
        .field("height", &type::height);

    EMSCRIPTEN_CV_SIZE(Size)
    EMSCRIPTEN_CV_SIZE(Size2f)

#define EMSCRIPTEN_CV_POINT(type) \
    emscripten::value_object<type>("#type") \
        .field("x", &type::x) \
        .field("y", &type::y); \

    EMSCRIPTEN_CV_POINT(Point)
    EMSCRIPTEN_CV_POINT(Point2f)

#define EMSCRIPTEN_CV_RECT(type, name) \
    emscripten::value_object<cv::Rect_<type>> (name) \
        .field("x", &cv::Rect_<type>::x) \
        .field("y", &cv::Rect_<type>::y) \
        .field("width", &cv::Rect_<type>::width) \
        .field("height", &cv::Rect_<type>::height);

    EMSCRIPTEN_CV_RECT(int, "Rect")
    EMSCRIPTEN_CV_RECT(float, "Rect2f")

    emscripten::value_object<cv::RotatedRect>("RotatedRect")
        .field("center", &cv::RotatedRect::center)
        .field("size", &cv::RotatedRect::size)
        .field("angle", &cv::RotatedRect::angle);

    function("rotatedRectPoints", select_overload<emscripten::val(const cv::RotatedRect&)>(&binding_utils::rotatedRectPoints));
    function("rotatedRectBoundingRect", select_overload<Rect(const cv::RotatedRect&)>(&binding_utils::rotatedRectBoundingRect));
    function("rotatedRectBoundingRect2f", select_overload<Rect2f(const cv::RotatedRect&)>(&binding_utils::rotatedRectBoundingRect2f));

    emscripten::value_object<cv::KeyPoint>("KeyPoint")
        .field("angle", &cv::KeyPoint::angle)
        .field("class_id", &cv::KeyPoint::class_id)
        .field("octave", &cv::KeyPoint::octave)
        .field("pt", &cv::KeyPoint::pt)
        .field("response", &cv::KeyPoint::response)
        .field("size", &cv::KeyPoint::size);

    emscripten::value_object<cv::DMatch>("DMatch")
        .field("queryIdx", &cv::DMatch::queryIdx)
        .field("trainIdx", &cv::DMatch::trainIdx)
        .field("imgIdx", &cv::DMatch::imgIdx)
        .field("distance", &cv::DMatch::distance);

    emscripten::value_array<cv::Scalar_<double>> ("Scalar")
        .element(emscripten::index<0>())
        .element(emscripten::index<1>())
        .element(emscripten::index<2>())
        .element(emscripten::index<3>());

    emscripten::value_object<binding_utils::MinMaxLoc>("MinMaxLoc")
        .field("minVal", &binding_utils::MinMaxLoc::minVal)
        .field("maxVal", &binding_utils::MinMaxLoc::maxVal)
        .field("minLoc", &binding_utils::MinMaxLoc::minLoc)
        .field("maxLoc", &binding_utils::MinMaxLoc::maxLoc);

    emscripten::value_object<binding_utils::Circle>("Circle")
        .field("center", &binding_utils::Circle::center)
        .field("radius", &binding_utils::Circle::radius);

    emscripten::value_object<cv::Moments >("Moments")
        .field("m00", &cv::Moments::m00)
        .field("m10", &cv::Moments::m10)
        .field("m01", &cv::Moments::m01)
        .field("m20", &cv::Moments::m20)
        .field("m11", &cv::Moments::m11)
        .field("m02", &cv::Moments::m02)
        .field("m30", &cv::Moments::m30)
        .field("m21", &cv::Moments::m21)
        .field("m12", &cv::Moments::m12)
        .field("m03", &cv::Moments::m03)
        .field("mu20", &cv::Moments::mu20)
        .field("mu11", &cv::Moments::mu11)
        .field("mu02", &cv::Moments::mu02)
        .field("mu30", &cv::Moments::mu30)
        .field("mu21", &cv::Moments::mu21)
        .field("mu12", &cv::Moments::mu12)
        .field("mu03", &cv::Moments::mu03)
        .field("nu20", &cv::Moments::nu20)
        .field("nu11", &cv::Moments::nu11)
        .field("nu02", &cv::Moments::nu02)
        .field("nu30", &cv::Moments::nu30)
        .field("nu21", &cv::Moments::nu21)
        .field("nu12", &cv::Moments::nu12)
        .field("nu03", &cv::Moments::nu03);

    emscripten::value_object<cv::Exception>("Exception")
        .field("code", &cv::Exception::code)
        .field("msg", &binding_utils::getExceptionMsg, &binding_utils::setExceptionMsg);

    function("exceptionFromPtr", &binding_utils::exceptionFromPtr, allow_raw_pointers());

#ifdef HAVE_OPENCV_IMGPROC
    function("minEnclosingCircle", select_overload<binding_utils::Circle(const cv::Mat&)>(&binding_utils::minEnclosingCircle));

    function("floodFill", select_overload<int(cv::Mat&, cv::Mat&, Point, Scalar, emscripten::val, Scalar, Scalar, int)>(&binding_utils::floodFill_wrapper));

    function("floodFill", select_overload<int(cv::Mat&, cv::Mat&, Point, Scalar, emscripten::val, Scalar, Scalar)>(&binding_utils::floodFill_wrapper_1));

    function("floodFill", select_overload<int(cv::Mat&, cv::Mat&, Point, Scalar, emscripten::val, Scalar)>(&binding_utils::floodFill_wrapper_2));

    function("floodFill", select_overload<int(cv::Mat&, cv::Mat&, Point, Scalar, emscripten::val)>(&binding_utils::floodFill_wrapper_3));

    function("floodFill", select_overload<int(cv::Mat&, cv::Mat&, Point, Scalar)>(&binding_utils::floodFill_wrapper_4));
#endif

    function("minMaxLoc", select_overload<binding_utils::MinMaxLoc(const cv::Mat&, const cv::Mat&)>(&binding_utils::minMaxLoc));

    function("minMaxLoc", select_overload<binding_utils::MinMaxLoc(const cv::Mat&)>(&binding_utils::minMaxLoc_1));

#ifdef HAVE_OPENCV_IMGPROC
    function("morphologyDefaultBorderValue", &cv::morphologyDefaultBorderValue);
#endif

    function("CV_MAT_DEPTH", &binding_utils::cvMatDepth);

#ifdef HAVE_OPENCV_VIDEO
    function("CamShift", select_overload<emscripten::val(const cv::Mat&, Rect&, TermCriteria)>(&binding_utils::CamShiftWrapper));

    function("meanShift", select_overload<emscripten::val(const cv::Mat&, Rect&, TermCriteria)>(&binding_utils::meanShiftWrapper));

    emscripten::class_<cv::Tracker >("Tracker")
        .function("init", select_overload<void(cv::Tracker&,const cv::Mat&,const Rect&)>(&binding_utils::Tracker_init_wrapper), pure_virtual())
        .function("update", select_overload<emscripten::val(cv::Tracker&,const cv::Mat&)>(&binding_utils::Tracker_update_wrapper), pure_virtual());

#endif

    function("getBuildInformation", &binding_utils::getBuildInformation);

#ifdef HAVE_PTHREADS_PF
    function("parallel_pthreads_set_threads_num", &cv::parallel_pthreads_set_threads_num);
    function("parallel_pthreads_get_threads_num", &cv::parallel_pthreads_get_threads_num);
#endif

#ifdef TEST_WASM_INTRIN
    function("test_hal_intrin_uint8", &binding_utils::test_hal_intrin_uint8);
    function("test_hal_intrin_int8", &binding_utils::test_hal_intrin_int8);
    function("test_hal_intrin_uint16", &binding_utils::test_hal_intrin_uint16);
    function("test_hal_intrin_int16", &binding_utils::test_hal_intrin_int16);
    function("test_hal_intrin_uint32", &binding_utils::test_hal_intrin_uint32);
    function("test_hal_intrin_int32", &binding_utils::test_hal_intrin_int32);
    function("test_hal_intrin_uint64", &binding_utils::test_hal_intrin_uint64);
    function("test_hal_intrin_int64", &binding_utils::test_hal_intrin_int64);
    function("test_hal_intrin_float32", &binding_utils::test_hal_intrin_float32);
    function("test_hal_intrin_float64", &binding_utils::test_hal_intrin_float64);
    function("test_hal_intrin_all", &binding_utils::test_hal_intrin_all);
#endif

    constant("CV_8UC1", CV_8UC1);
    constant("CV_8UC2", CV_8UC2);
    constant("CV_8UC3", CV_8UC3);
    constant("CV_8UC4", CV_8UC4);

    constant("CV_8SC1", CV_8SC1);
    constant("CV_8SC2", CV_8SC2);
    constant("CV_8SC3", CV_8SC3);
    constant("CV_8SC4", CV_8SC4);

    constant("CV_16UC1", CV_16UC1);
    constant("CV_16UC2", CV_16UC2);
    constant("CV_16UC3", CV_16UC3);
    constant("CV_16UC4", CV_16UC4);

    constant("CV_16SC1", CV_16SC1);
    constant("CV_16SC2", CV_16SC2);
    constant("CV_16SC3", CV_16SC3);
    constant("CV_16SC4", CV_16SC4);

    constant("CV_32SC1", CV_32SC1);
    constant("CV_32SC2", CV_32SC2);
    constant("CV_32SC3", CV_32SC3);
    constant("CV_32SC4", CV_32SC4);

    constant("CV_32FC1", CV_32FC1);
    constant("CV_32FC2", CV_32FC2);
    constant("CV_32FC3", CV_32FC3);
    constant("CV_32FC4", CV_32FC4);

    constant("CV_64FC1", CV_64FC1);
    constant("CV_64FC2", CV_64FC2);
    constant("CV_64FC3", CV_64FC3);
    constant("CV_64FC4", CV_64FC4);

    constant("CV_8U", CV_8U);
    constant("CV_8S", CV_8S);
    constant("CV_16U", CV_16U);
    constant("CV_16S", CV_16S);
    constant("CV_32S",  CV_32S);
    constant("CV_32F", CV_32F);
    constant("CV_64F", CV_64F);

    constant("INT_MIN", INT_MIN);
    constant("INT_MAX", INT_MAX);
}
namespace Wrappers {
    void LUT_wrapper(const cv::Mat& arg1, const cv::Mat& arg2, cv::Mat& arg3) {
        return cv::LUT(arg1, arg2, arg3);
    }
    
    void Rodrigues_wrapper(const cv::Mat& arg1, cv::Mat& arg2, cv::Mat& arg3) {
        return cv::Rodrigues(arg1, arg2, arg3);
    }
    
    void Rodrigues_wrapper_1(const cv::Mat& arg1, cv::Mat& arg2) {
        return cv::Rodrigues(arg1, arg2);
    }
    
    void absdiff_wrapper(const cv::Mat& arg1, const cv::Mat& arg2, cv::Mat& arg3) {
        return cv::absdiff(arg1, arg2, arg3);
    }
    
    void add_wrapper(const cv::Mat& arg1, const cv::Mat& arg2, cv::Mat& arg3, const cv::Mat& arg4, int arg5) {
        return cv::add(arg1, arg2, arg3, arg4, arg5);
    }
    
    void add_wrapper_1(const cv::Mat& arg1, const cv::Mat& arg2, cv::Mat& arg3, const cv::Mat& arg4) {
        return cv::add(arg1, arg2, arg3, arg4);
    }
    
    void add_wrapper_2(const cv::Mat& arg1, const cv::Mat& arg2, cv::Mat& arg3) {
        return cv::add(arg1, arg2, arg3);
    }
    
    void addWeighted_wrapper(const cv::Mat& arg1, double arg2, const cv::Mat& arg3, double arg4, double arg5, cv::Mat& arg6, int arg7) {
        return cv::addWeighted(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
    }
    
    void addWeighted_wrapper_1(const cv::Mat& arg1, double arg2, const cv::Mat& arg3, double arg4, double arg5, cv::Mat& arg6) {
        return cv::addWeighted(arg1, arg2, arg3, arg4, arg5, arg6);
    }
    
    void bitwise_and_wrapper(const cv::Mat& arg1, const cv::Mat& arg2, cv::Mat& arg3, const cv::Mat& arg4) {
        return cv::bitwise_and(arg1, arg2, arg3, arg4);
    }
    
    void bitwise_and_wrapper_1(const cv::Mat& arg1, const cv::Mat& arg2, cv::Mat& arg3) {
        return cv::bitwise_and(arg1, arg2, arg3);
    }
    
    void bitwise_not_wrapper(const cv::Mat& arg1, cv::Mat& arg2, const cv::Mat& arg3) {
        return cv::bitwise_not(arg1, arg2, arg3);
    }
    
    void bitwise_not_wrapper_1(const cv::Mat& arg1, cv::Mat& arg2) {
        return cv::bitwise_not(arg1, arg2);
    }
    
    void bitwise_or_wrapper(const cv::Mat& arg1, const cv::Mat& arg2, cv::Mat& arg3, const cv::Mat& arg4) {
        return cv::bitwise_or(arg1, arg2, arg3, arg4);
    }
    
    void bitwise_or_wrapper_1(const cv::Mat& arg1, const cv::Mat& arg2, cv::Mat& arg3) {
        return cv::bitwise_or(arg1, arg2, arg3);
    }
    
    void bitwise_xor_wrapper(const cv::Mat& arg1, const cv::Mat& arg2, cv::Mat& arg3, const cv::Mat& arg4) {
        return cv::bitwise_xor(arg1, arg2, arg3, arg4);
    }
    
    void bitwise_xor_wrapper_1(const cv::Mat& arg1, const cv::Mat& arg2, cv::Mat& arg3) {
        return cv::bitwise_xor(arg1, arg2, arg3);
    }
    
    double calibrateCameraExtended_wrapper(const std::vector<cv::Mat>& arg1, const std::vector<cv::Mat>& arg2, Size arg3, cv::Mat& arg4, cv::Mat& arg5, std::vector<cv::Mat>& arg6, std::vector<cv::Mat>& arg7, cv::Mat& arg8, cv::Mat& arg9, cv::Mat& arg10, int arg11, TermCriteria arg12) {
        return cv::calibrateCamera(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12);
    }
    
    double calibrateCameraExtended_wrapper_1(const std::vector<cv::Mat>& arg1, const std::vector<cv::Mat>& arg2, Size arg3, cv::Mat& arg4, cv::Mat& arg5, std::vector<cv::Mat>& arg6, std::vector<cv::Mat>& arg7, cv::Mat& arg8, cv::Mat& arg9, cv::Mat& arg10, int arg11) {
        return cv::calibrateCamera(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11);
    }
    
    double calibrateCameraExtended_wrapper_2(const std::vector<cv::Mat>& arg1, const std::vector<cv::Mat>& arg2, Size arg3, cv::Mat& arg4, cv::Mat& arg5, std::vector<cv::Mat>& arg6, std::vector<cv::Mat>& arg7, cv::Mat& arg8, cv::Mat& arg9, cv::Mat& arg10) {
        return cv::calibrateCamera(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10);
    }
    
    void cartToPolar_wrapper(const cv::Mat& arg1, const cv::Mat& arg2, cv::Mat& arg3, cv::Mat& arg4, bool arg5) {
        return cv::cartToPolar(arg1, arg2, arg3, arg4, arg5);
    }
    
    void cartToPolar_wrapper_1(const cv::Mat& arg1, const cv::Mat& arg2, cv::Mat& arg3, cv::Mat& arg4) {
        return cv::cartToPolar(arg1, arg2, arg3, arg4);
    }
    
    void compare_wrapper(const cv::Mat& arg1, const cv::Mat& arg2, cv::Mat& arg3, int arg4) {
        return cv::compare(arg1, arg2, arg3, arg4);
    }
    
    void convertScaleAbs_wrapper(const cv::Mat& arg1, cv::Mat& arg2, double arg3, double arg4) {
        return cv::convertScaleAbs(arg1, arg2, arg3, arg4);
    }
    
    void convertScaleAbs_wrapper_1(const cv::Mat& arg1, cv::Mat& arg2, double arg3) {
        return cv::convertScaleAbs(arg1, arg2, arg3);
    }
    
    void convertScaleAbs_wrapper_2(const cv::Mat& arg1, cv::Mat& arg2) {
        return cv::convertScaleAbs(arg1, arg2);
    }
    
    void copyMakeBorder_wrapper(const cv::Mat& arg1, cv::Mat& arg2, int arg3, int arg4, int arg5, int arg6, int arg7, const Scalar& arg8) {
        return cv::copyMakeBorder(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
    }
    
    void copyMakeBorder_wrapper_1(const cv::Mat& arg1, cv::Mat& arg2, int arg3, int arg4, int arg5, int arg6, int arg7) {
        return cv::copyMakeBorder(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
    }
    
    int countNonZero_wrapper(const cv::Mat& arg1) {
        return cv::countNonZero(arg1);
    }
    
    double determinant_wrapper(const cv::Mat& arg1) {
        return cv::determinant(arg1);
    }
    
    void dft_wrapper(const cv::Mat& arg1, cv::Mat& arg2, int arg3, int arg4) {
        return cv::dft(arg1, arg2, arg3, arg4);
    }
    
    void dft_wrapper_1(const cv::Mat& arg1, cv::Mat& arg2, int arg3) {
        return cv::dft(arg1, arg2, arg3);
    }
    
    void dft_wrapper_2(const cv::Mat& arg1, cv::Mat& arg2) {
        return cv::dft(arg1, arg2);
    }
    
    void divide_wrapper(const cv::Mat& arg1, const cv::Mat& arg2, cv::Mat& arg3, double arg4, int arg5) {
        return cv::divide(arg1, arg2, arg3, arg4, arg5);
    }
    
    void divide_wrapper_1(const cv::Mat& arg1, const cv::Mat& arg2, cv::Mat& arg3, double arg4) {
        return cv::divide(arg1, arg2, arg3, arg4);
    }
    
    void divide_wrapper_2(const cv::Mat& arg1, const cv::Mat& arg2, cv::Mat& arg3) {
        return cv::divide(arg1, arg2, arg3);
    }
    
    void divide_wrapper1(double arg1, const cv::Mat& arg2, cv::Mat& arg3, int arg4) {
        return cv::divide(arg1, arg2, arg3, arg4);
    }
    
    void divide_wrapper1_1(double arg1, const cv::Mat& arg2, cv::Mat& arg3) {
        return cv::divide(arg1, arg2, arg3);
    }
    
    void drawFrameAxes_wrapper(cv::Mat& arg1, const cv::Mat& arg2, const cv::Mat& arg3, const cv::Mat& arg4, const cv::Mat& arg5, float arg6, int arg7) {
        return cv::drawFrameAxes(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
    }
    
    void drawFrameAxes_wrapper_1(cv::Mat& arg1, const cv::Mat& arg2, const cv::Mat& arg3, const cv::Mat& arg4, const cv::Mat& arg5, float arg6) {
        return cv::drawFrameAxes(arg1, arg2, arg3, arg4, arg5, arg6);
    }
    
    bool eigen_wrapper(const cv::Mat& arg1, cv::Mat& arg2, cv::Mat& arg3) {
        return cv::eigen(arg1, arg2, arg3);
    }
    
    bool eigen_wrapper_1(const cv::Mat& arg1, cv::Mat& arg2) {
        return cv::eigen(arg1, arg2);
    }
    
    Mat estimateAffine2D_wrapper(const cv::Mat& arg1, const cv::Mat& arg2, cv::Mat& arg3, int arg4, double arg5, size_t arg6, double arg7, size_t arg8) {
        return cv::estimateAffine2D(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
    }
    
    Mat estimateAffine2D_wrapper_1(const cv::Mat& arg1, const cv::Mat& arg2, cv::Mat& arg3, int arg4, double arg5, size_t arg6, double arg7) {
        return cv::estimateAffine2D(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
    }
    
    Mat estimateAffine2D_wrapper_2(const cv::Mat& arg1, const cv::Mat& arg2, cv::Mat& arg3, int arg4, double arg5, size_t arg6) {
        return cv::estimateAffine2D(arg1, arg2, arg3, arg4, arg5, arg6);
    }
    
    Mat estimateAffine2D_wrapper_3(const cv::Mat& arg1, const cv::Mat& arg2, cv::Mat& arg3, int arg4, double arg5) {
        return cv::estimateAffine2D(arg1, arg2, arg3, arg4, arg5);
    }
    
    Mat estimateAffine2D_wrapper_4(const cv::Mat& arg1, const cv::Mat& arg2, cv::Mat& arg3, int arg4) {
        return cv::estimateAffine2D(arg1, arg2, arg3, arg4);
    }
    
    Mat estimateAffine2D_wrapper_5(const cv::Mat& arg1, const cv::Mat& arg2, cv::Mat& arg3) {
        return cv::estimateAffine2D(arg1, arg2, arg3);
    }
    
    Mat estimateAffine2D_wrapper_6(const cv::Mat& arg1, const cv::Mat& arg2) {
        return cv::estimateAffine2D(arg1, arg2);
    }
    
    Mat estimateAffine2D_wrapper1(const cv::Mat& arg1, const cv::Mat& arg2, cv::Mat& arg3, const UsacParams& arg4) {
        return cv::estimateAffine2D(arg1, arg2, arg3, arg4);
    }
    
    void exp_wrapper(const cv::Mat& arg1, cv::Mat& arg2) {
        return cv::exp(arg1, arg2);
    }
    
    Mat findHomography_wrapper(const cv::Mat& arg1, const cv::Mat& arg2, int arg3, double arg4, cv::Mat& arg5, const int arg6, const double arg7) {
        return cv::findHomography(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
    }
    
    Mat findHomography_wrapper_1(const cv::Mat& arg1, const cv::Mat& arg2, int arg3, double arg4, cv::Mat& arg5, const int arg6) {
        return cv::findHomography(arg1, arg2, arg3, arg4, arg5, arg6);
    }
    
    Mat findHomography_wrapper_2(const cv::Mat& arg1, const cv::Mat& arg2, int arg3, double arg4, cv::Mat& arg5) {
        return cv::findHomography(arg1, arg2, arg3, arg4, arg5);
    }
    
    Mat findHomography_wrapper_3(const cv::Mat& arg1, const cv::Mat& arg2, int arg3, double arg4) {
        return cv::findHomography(arg1, arg2, arg3, arg4);
    }
    
    Mat findHomography_wrapper_4(const cv::Mat& arg1, const cv::Mat& arg2, int arg3) {
        return cv::findHomography(arg1, arg2, arg3);
    }
    
    Mat findHomography_wrapper_5(const cv::Mat& arg1, const cv::Mat& arg2) {
        return cv::findHomography(arg1, arg2);
    }
    
    Mat findHomography_wrapper1(const cv::Mat& arg1, const cv::Mat& arg2, cv::Mat& arg3, const UsacParams& arg4) {
        return cv::findHomography(arg1, arg2, arg3, arg4);
    }
    
    void flip_wrapper(const cv::Mat& arg1, cv::Mat& arg2, int arg3) {
        return cv::flip(arg1, arg2, arg3);
    }
    
    void gemm_wrapper(const cv::Mat& arg1, const cv::Mat& arg2, double arg3, const cv::Mat& arg4, double arg5, cv::Mat& arg6, int arg7) {
        return cv::gemm(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
    }
    
    void gemm_wrapper_1(const cv::Mat& arg1, const cv::Mat& arg2, double arg3, const cv::Mat& arg4, double arg5, cv::Mat& arg6) {
        return cv::gemm(arg1, arg2, arg3, arg4, arg5, arg6);
    }
    
    Mat getDefaultNewCameraMatrix_wrapper(const cv::Mat& arg1, Size arg2, bool arg3) {
        return cv::getDefaultNewCameraMatrix(arg1, arg2, arg3);
    }
    
    Mat getDefaultNewCameraMatrix_wrapper_1(const cv::Mat& arg1, Size arg2) {
        return cv::getDefaultNewCameraMatrix(arg1, arg2);
    }
    
    Mat getDefaultNewCameraMatrix_wrapper_2(const cv::Mat& arg1) {
        return cv::getDefaultNewCameraMatrix(arg1);
    }
    
    int getLogLevel_wrapper() {
        return cv::getLogLevel();
    }
    
    int getOptimalDFTSize_wrapper(int arg1) {
        return cv::getOptimalDFTSize(arg1);
    }
    
    void hconcat_wrapper(const std::vector<cv::Mat>& arg1, cv::Mat& arg2) {
        return cv::hconcat(arg1, arg2);
    }
    
    void inRange_wrapper(const cv::Mat& arg1, const cv::Mat& arg2, const cv::Mat& arg3, cv::Mat& arg4) {
        return cv::inRange(arg1, arg2, arg3, arg4);
    }
    
    void initUndistortRectifyMap_wrapper(const cv::Mat& arg1, const cv::Mat& arg2, const cv::Mat& arg3, const cv::Mat& arg4, Size arg5, int arg6, cv::Mat& arg7, cv::Mat& arg8) {
        return cv::initUndistortRectifyMap(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
    }
    
    double invert_wrapper(const cv::Mat& arg1, cv::Mat& arg2, int arg3) {
        return cv::invert(arg1, arg2, arg3);
    }
    
    double invert_wrapper_1(const cv::Mat& arg1, cv::Mat& arg2) {
        return cv::invert(arg1, arg2);
    }
    
    double kmeans_wrapper(const cv::Mat& arg1, int arg2, cv::Mat& arg3, TermCriteria arg4, int arg5, int arg6, cv::Mat& arg7) {
        return cv::kmeans(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
    }
    
    double kmeans_wrapper_1(const cv::Mat& arg1, int arg2, cv::Mat& arg3, TermCriteria arg4, int arg5, int arg6) {
        return cv::kmeans(arg1, arg2, arg3, arg4, arg5, arg6);
    }
    
    void log_wrapper(const cv::Mat& arg1, cv::Mat& arg2) {
        return cv::log(arg1, arg2);
    }
    
    void magnitude_wrapper(const cv::Mat& arg1, const cv::Mat& arg2, cv::Mat& arg3) {
        return cv::magnitude(arg1, arg2, arg3);
    }
    
    void max_wrapper(const cv::Mat& arg1, const cv::Mat& arg2, cv::Mat& arg3) {
        return cv::max(arg1, arg2, arg3);
    }
    
    Scalar mean_wrapper(const cv::Mat& arg1, const cv::Mat& arg2) {
        return cv::mean(arg1, arg2);
    }
    
    Scalar mean_wrapper_1(const cv::Mat& arg1) {
        return cv::mean(arg1);
    }
    
    void meanStdDev_wrapper(const cv::Mat& arg1, cv::Mat& arg2, cv::Mat& arg3, const cv::Mat& arg4) {
        return cv::meanStdDev(arg1, arg2, arg3, arg4);
    }
    
    void meanStdDev_wrapper_1(const cv::Mat& arg1, cv::Mat& arg2, cv::Mat& arg3) {
        return cv::meanStdDev(arg1, arg2, arg3);
    }
    
    void merge_wrapper(const std::vector<cv::Mat>& arg1, cv::Mat& arg2) {
        return cv::merge(arg1, arg2);
    }
    
    void min_wrapper(const cv::Mat& arg1, const cv::Mat& arg2, cv::Mat& arg3) {
        return cv::min(arg1, arg2, arg3);
    }
    
    void mixChannels_wrapper(const std::vector<cv::Mat>& arg1, std::vector<cv::Mat>& arg2, const emscripten::val& arg3) {
        return cv::mixChannels(arg1, arg2, emscripten::vecFromJSArray<int>(arg3));
    }
    
    void multiply_wrapper(const cv::Mat& arg1, const cv::Mat& arg2, cv::Mat& arg3, double arg4, int arg5) {
        return cv::multiply(arg1, arg2, arg3, arg4, arg5);
    }
    
    void multiply_wrapper_1(const cv::Mat& arg1, const cv::Mat& arg2, cv::Mat& arg3, double arg4) {
        return cv::multiply(arg1, arg2, arg3, arg4);
    }
    
    void multiply_wrapper_2(const cv::Mat& arg1, const cv::Mat& arg2, cv::Mat& arg3) {
        return cv::multiply(arg1, arg2, arg3);
    }
    
    double norm_wrapper(const cv::Mat& arg1, int arg2, const cv::Mat& arg3) {
        return cv::norm(arg1, arg2, arg3);
    }
    
    double norm_wrapper_1(const cv::Mat& arg1, int arg2) {
        return cv::norm(arg1, arg2);
    }
    
    double norm_wrapper_2(const cv::Mat& arg1) {
        return cv::norm(arg1);
    }
    
    double norm_wrapper1(const cv::Mat& arg1, const cv::Mat& arg2, int arg3, const cv::Mat& arg4) {
        return cv::norm(arg1, arg2, arg3, arg4);
    }
    
    double norm_wrapper1_1(const cv::Mat& arg1, const cv::Mat& arg2, int arg3) {
        return cv::norm(arg1, arg2, arg3);
    }
    
    double norm_wrapper1_2(const cv::Mat& arg1, const cv::Mat& arg2) {
        return cv::norm(arg1, arg2);
    }
    
    void normalize_wrapper(const cv::Mat& arg1, cv::Mat& arg2, double arg3, double arg4, int arg5, int arg6, const cv::Mat& arg7) {
        return cv::normalize(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
    }
    
    void normalize_wrapper_1(const cv::Mat& arg1, cv::Mat& arg2, double arg3, double arg4, int arg5, int arg6) {
        return cv::normalize(arg1, arg2, arg3, arg4, arg5, arg6);
    }
    
    void normalize_wrapper_2(const cv::Mat& arg1, cv::Mat& arg2, double arg3, double arg4, int arg5) {
        return cv::normalize(arg1, arg2, arg3, arg4, arg5);
    }
    
    void normalize_wrapper_3(const cv::Mat& arg1, cv::Mat& arg2, double arg3, double arg4) {
        return cv::normalize(arg1, arg2, arg3, arg4);
    }
    
    void normalize_wrapper_4(const cv::Mat& arg1, cv::Mat& arg2, double arg3) {
        return cv::normalize(arg1, arg2, arg3);
    }
    
    void normalize_wrapper_5(const cv::Mat& arg1, cv::Mat& arg2) {
        return cv::normalize(arg1, arg2);
    }
    
    void perspectiveTransform_wrapper(const cv::Mat& arg1, cv::Mat& arg2, const cv::Mat& arg3) {
        return cv::perspectiveTransform(arg1, arg2, arg3);
    }
    
    void polarToCart_wrapper(const cv::Mat& arg1, const cv::Mat& arg2, cv::Mat& arg3, cv::Mat& arg4, bool arg5) {
        return cv::polarToCart(arg1, arg2, arg3, arg4, arg5);
    }
    
    void polarToCart_wrapper_1(const cv::Mat& arg1, const cv::Mat& arg2, cv::Mat& arg3, cv::Mat& arg4) {
        return cv::polarToCart(arg1, arg2, arg3, arg4);
    }
    
    void pow_wrapper(const cv::Mat& arg1, double arg2, cv::Mat& arg3) {
        return cv::pow(arg1, arg2, arg3);
    }
    
    void projectPoints_wrapper(const cv::Mat& arg1, const cv::Mat& arg2, const cv::Mat& arg3, const cv::Mat& arg4, const cv::Mat& arg5, cv::Mat& arg6, cv::Mat& arg7, double arg8) {
        return cv::projectPoints(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
    }
    
    void projectPoints_wrapper_1(const cv::Mat& arg1, const cv::Mat& arg2, const cv::Mat& arg3, const cv::Mat& arg4, const cv::Mat& arg5, cv::Mat& arg6, cv::Mat& arg7) {
        return cv::projectPoints(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
    }
    
    void projectPoints_wrapper_2(const cv::Mat& arg1, const cv::Mat& arg2, const cv::Mat& arg3, const cv::Mat& arg4, const cv::Mat& arg5, cv::Mat& arg6) {
        return cv::projectPoints(arg1, arg2, arg3, arg4, arg5, arg6);
    }
    
    void randn_wrapper(cv::Mat& arg1, const cv::Mat& arg2, const cv::Mat& arg3) {
        return cv::randn(arg1, arg2, arg3);
    }
    
    void randu_wrapper(cv::Mat& arg1, const cv::Mat& arg2, const cv::Mat& arg3) {
        return cv::randu(arg1, arg2, arg3);
    }
    
    void reduce_wrapper(const cv::Mat& arg1, cv::Mat& arg2, int arg3, int arg4, int arg5) {
        return cv::reduce(arg1, arg2, arg3, arg4, arg5);
    }
    
    void reduce_wrapper_1(const cv::Mat& arg1, cv::Mat& arg2, int arg3, int arg4) {
        return cv::reduce(arg1, arg2, arg3, arg4);
    }
    
    void repeat_wrapper(const cv::Mat& arg1, int arg2, int arg3, cv::Mat& arg4) {
        return cv::repeat(arg1, arg2, arg3, arg4);
    }
    
    void rotate_wrapper(const cv::Mat& arg1, cv::Mat& arg2, int arg3) {
        return cv::rotate(arg1, arg2, arg3);
    }
    
    void setIdentity_wrapper(cv::Mat& arg1, const Scalar& arg2) {
        return cv::setIdentity(arg1, arg2);
    }
    
    void setIdentity_wrapper_1(cv::Mat& arg1) {
        return cv::setIdentity(arg1);
    }
    
    int setLogLevel_wrapper(int arg1) {
        return cv::setLogLevel(arg1);
    }
    
    void setRNGSeed_wrapper(int arg1) {
        return cv::setRNGSeed(arg1);
    }
    
    bool solve_wrapper(const cv::Mat& arg1, const cv::Mat& arg2, cv::Mat& arg3, int arg4) {
        return cv::solve(arg1, arg2, arg3, arg4);
    }
    
    bool solve_wrapper_1(const cv::Mat& arg1, const cv::Mat& arg2, cv::Mat& arg3) {
        return cv::solve(arg1, arg2, arg3);
    }
    
    bool solvePnP_wrapper(const cv::Mat& arg1, const cv::Mat& arg2, const cv::Mat& arg3, const cv::Mat& arg4, cv::Mat& arg5, cv::Mat& arg6, bool arg7, int arg8) {
        return cv::solvePnP(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
    }
    
    bool solvePnP_wrapper_1(const cv::Mat& arg1, const cv::Mat& arg2, const cv::Mat& arg3, const cv::Mat& arg4, cv::Mat& arg5, cv::Mat& arg6, bool arg7) {
        return cv::solvePnP(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
    }
    
    bool solvePnP_wrapper_2(const cv::Mat& arg1, const cv::Mat& arg2, const cv::Mat& arg3, const cv::Mat& arg4, cv::Mat& arg5, cv::Mat& arg6) {
        return cv::solvePnP(arg1, arg2, arg3, arg4, arg5, arg6);
    }
    
    bool solvePnPRansac_wrapper(const cv::Mat& arg1, const cv::Mat& arg2, const cv::Mat& arg3, const cv::Mat& arg4, cv::Mat& arg5, cv::Mat& arg6, bool arg7, int arg8, float arg9, double arg10, cv::Mat& arg11, int arg12) {
        return cv::solvePnPRansac(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12);
    }
    
    bool solvePnPRansac_wrapper_1(const cv::Mat& arg1, const cv::Mat& arg2, const cv::Mat& arg3, const cv::Mat& arg4, cv::Mat& arg5, cv::Mat& arg6, bool arg7, int arg8, float arg9, double arg10, cv::Mat& arg11) {
        return cv::solvePnPRansac(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11);
    }
    
    bool solvePnPRansac_wrapper_2(const cv::Mat& arg1, const cv::Mat& arg2, const cv::Mat& arg3, const cv::Mat& arg4, cv::Mat& arg5, cv::Mat& arg6, bool arg7, int arg8, float arg9, double arg10) {
        return cv::solvePnPRansac(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10);
    }
    
    bool solvePnPRansac_wrapper_3(const cv::Mat& arg1, const cv::Mat& arg2, const cv::Mat& arg3, const cv::Mat& arg4, cv::Mat& arg5, cv::Mat& arg6, bool arg7, int arg8, float arg9) {
        return cv::solvePnPRansac(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
    }
    
    bool solvePnPRansac_wrapper_4(const cv::Mat& arg1, const cv::Mat& arg2, const cv::Mat& arg3, const cv::Mat& arg4, cv::Mat& arg5, cv::Mat& arg6, bool arg7, int arg8) {
        return cv::solvePnPRansac(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
    }
    
    bool solvePnPRansac_wrapper_5(const cv::Mat& arg1, const cv::Mat& arg2, const cv::Mat& arg3, const cv::Mat& arg4, cv::Mat& arg5, cv::Mat& arg6, bool arg7) {
        return cv::solvePnPRansac(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
    }
    
    bool solvePnPRansac_wrapper_6(const cv::Mat& arg1, const cv::Mat& arg2, const cv::Mat& arg3, const cv::Mat& arg4, cv::Mat& arg5, cv::Mat& arg6) {
        return cv::solvePnPRansac(arg1, arg2, arg3, arg4, arg5, arg6);
    }
    
    bool solvePnPRansac_wrapper1(const cv::Mat& arg1, const cv::Mat& arg2, cv::Mat& arg3, const cv::Mat& arg4, cv::Mat& arg5, cv::Mat& arg6, cv::Mat& arg7, const UsacParams& arg8) {
        return cv::solvePnPRansac(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
    }
    
    bool solvePnPRansac_wrapper1_1(const cv::Mat& arg1, const cv::Mat& arg2, cv::Mat& arg3, const cv::Mat& arg4, cv::Mat& arg5, cv::Mat& arg6, cv::Mat& arg7) {
        return cv::solvePnPRansac(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
    }
    
    void solvePnPRefineLM_wrapper(const cv::Mat& arg1, const cv::Mat& arg2, const cv::Mat& arg3, const cv::Mat& arg4, cv::Mat& arg5, cv::Mat& arg6, TermCriteria arg7) {
        return cv::solvePnPRefineLM(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
    }
    
    void solvePnPRefineLM_wrapper_1(const cv::Mat& arg1, const cv::Mat& arg2, const cv::Mat& arg3, const cv::Mat& arg4, cv::Mat& arg5, cv::Mat& arg6) {
        return cv::solvePnPRefineLM(arg1, arg2, arg3, arg4, arg5, arg6);
    }
    
    double solvePoly_wrapper(const cv::Mat& arg1, cv::Mat& arg2, int arg3) {
        return cv::solvePoly(arg1, arg2, arg3);
    }
    
    double solvePoly_wrapper_1(const cv::Mat& arg1, cv::Mat& arg2) {
        return cv::solvePoly(arg1, arg2);
    }
    
    void split_wrapper(const cv::Mat& arg1, std::vector<cv::Mat>& arg2) {
        return cv::split(arg1, arg2);
    }
    
    void sqrt_wrapper(const cv::Mat& arg1, cv::Mat& arg2) {
        return cv::sqrt(arg1, arg2);
    }
    
    void subtract_wrapper(const cv::Mat& arg1, const cv::Mat& arg2, cv::Mat& arg3, const cv::Mat& arg4, int arg5) {
        return cv::subtract(arg1, arg2, arg3, arg4, arg5);
    }
    
    void subtract_wrapper_1(const cv::Mat& arg1, const cv::Mat& arg2, cv::Mat& arg3, const cv::Mat& arg4) {
        return cv::subtract(arg1, arg2, arg3, arg4);
    }
    
    void subtract_wrapper_2(const cv::Mat& arg1, const cv::Mat& arg2, cv::Mat& arg3) {
        return cv::subtract(arg1, arg2, arg3);
    }
    
    Scalar trace_wrapper(const cv::Mat& arg1) {
        return cv::trace(arg1);
    }
    
    void transform_wrapper(const cv::Mat& arg1, cv::Mat& arg2, const cv::Mat& arg3) {
        return cv::transform(arg1, arg2, arg3);
    }
    
    void transpose_wrapper(const cv::Mat& arg1, cv::Mat& arg2) {
        return cv::transpose(arg1, arg2);
    }
    
    void undistort_wrapper(const cv::Mat& arg1, cv::Mat& arg2, const cv::Mat& arg3, const cv::Mat& arg4, const cv::Mat& arg5) {
        return cv::undistort(arg1, arg2, arg3, arg4, arg5);
    }
    
    void undistort_wrapper_1(const cv::Mat& arg1, cv::Mat& arg2, const cv::Mat& arg3, const cv::Mat& arg4) {
        return cv::undistort(arg1, arg2, arg3, arg4);
    }
    
    void vconcat_wrapper(const std::vector<cv::Mat>& arg1, cv::Mat& arg2) {
        return cv::vconcat(arg1, arg2);
    }
    
    void fisheye_initUndistortRectifyMap_wrapper(const cv::Mat& arg1, const cv::Mat& arg2, const cv::Mat& arg3, const cv::Mat& arg4, const Size& arg5, int arg6, cv::Mat& arg7, cv::Mat& arg8) {
        return cv::fisheye::initUndistortRectifyMap(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
    }
    
    void fisheye_projectPoints_wrapper(const cv::Mat& arg1, cv::Mat& arg2, const cv::Mat& arg3, const cv::Mat& arg4, const cv::Mat& arg5, const cv::Mat& arg6, double arg7, cv::Mat& arg8) {
        return cv::fisheye::projectPoints(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
    }
    
    void fisheye_projectPoints_wrapper_1(const cv::Mat& arg1, cv::Mat& arg2, const cv::Mat& arg3, const cv::Mat& arg4, const cv::Mat& arg5, const cv::Mat& arg6, double arg7) {
        return cv::fisheye::projectPoints(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
    }
    
    void fisheye_projectPoints_wrapper_2(const cv::Mat& arg1, cv::Mat& arg2, const cv::Mat& arg3, const cv::Mat& arg4, const cv::Mat& arg5, const cv::Mat& arg6) {
        return cv::fisheye::projectPoints(arg1, arg2, arg3, arg4, arg5, arg6);
    }
    
}

EMSCRIPTEN_BINDINGS(testBinding) {
    function("LUT", select_overload<void(const cv::Mat&, const cv::Mat&, cv::Mat&)>(&Wrappers::LUT_wrapper));

    function("Rodrigues", select_overload<void(const cv::Mat&, cv::Mat&, cv::Mat&)>(&Wrappers::Rodrigues_wrapper));

    function("Rodrigues", select_overload<void(const cv::Mat&, cv::Mat&)>(&Wrappers::Rodrigues_wrapper_1));

    function("absdiff", select_overload<void(const cv::Mat&, const cv::Mat&, cv::Mat&)>(&Wrappers::absdiff_wrapper));

    function("add", select_overload<void(const cv::Mat&, const cv::Mat&, cv::Mat&, const cv::Mat&, int)>(&Wrappers::add_wrapper));

    function("add", select_overload<void(const cv::Mat&, const cv::Mat&, cv::Mat&, const cv::Mat&)>(&Wrappers::add_wrapper_1));

    function("add", select_overload<void(const cv::Mat&, const cv::Mat&, cv::Mat&)>(&Wrappers::add_wrapper_2));

    function("addWeighted", select_overload<void(const cv::Mat&, double, const cv::Mat&, double, double, cv::Mat&, int)>(&Wrappers::addWeighted_wrapper));

    function("addWeighted", select_overload<void(const cv::Mat&, double, const cv::Mat&, double, double, cv::Mat&)>(&Wrappers::addWeighted_wrapper_1));

    function("bitwise_and", select_overload<void(const cv::Mat&, const cv::Mat&, cv::Mat&, const cv::Mat&)>(&Wrappers::bitwise_and_wrapper));

    function("bitwise_and", select_overload<void(const cv::Mat&, const cv::Mat&, cv::Mat&)>(&Wrappers::bitwise_and_wrapper_1));

    function("bitwise_not", select_overload<void(const cv::Mat&, cv::Mat&, const cv::Mat&)>(&Wrappers::bitwise_not_wrapper));

    function("bitwise_not", select_overload<void(const cv::Mat&, cv::Mat&)>(&Wrappers::bitwise_not_wrapper_1));

    function("bitwise_or", select_overload<void(const cv::Mat&, const cv::Mat&, cv::Mat&, const cv::Mat&)>(&Wrappers::bitwise_or_wrapper));

    function("bitwise_or", select_overload<void(const cv::Mat&, const cv::Mat&, cv::Mat&)>(&Wrappers::bitwise_or_wrapper_1));

    function("bitwise_xor", select_overload<void(const cv::Mat&, const cv::Mat&, cv::Mat&, const cv::Mat&)>(&Wrappers::bitwise_xor_wrapper));

    function("bitwise_xor", select_overload<void(const cv::Mat&, const cv::Mat&, cv::Mat&)>(&Wrappers::bitwise_xor_wrapper_1));

    function("calibrateCameraExtended", select_overload<double(const std::vector<cv::Mat>&, const std::vector<cv::Mat>&, Size, cv::Mat&, cv::Mat&, std::vector<cv::Mat>&, std::vector<cv::Mat>&, cv::Mat&, cv::Mat&, cv::Mat&, int, TermCriteria)>(&Wrappers::calibrateCameraExtended_wrapper));

    function("calibrateCameraExtended", select_overload<double(const std::vector<cv::Mat>&, const std::vector<cv::Mat>&, Size, cv::Mat&, cv::Mat&, std::vector<cv::Mat>&, std::vector<cv::Mat>&, cv::Mat&, cv::Mat&, cv::Mat&, int)>(&Wrappers::calibrateCameraExtended_wrapper_1));

    function("calibrateCameraExtended", select_overload<double(const std::vector<cv::Mat>&, const std::vector<cv::Mat>&, Size, cv::Mat&, cv::Mat&, std::vector<cv::Mat>&, std::vector<cv::Mat>&, cv::Mat&, cv::Mat&, cv::Mat&)>(&Wrappers::calibrateCameraExtended_wrapper_2));

    function("cartToPolar", select_overload<void(const cv::Mat&, const cv::Mat&, cv::Mat&, cv::Mat&, bool)>(&Wrappers::cartToPolar_wrapper));

    function("cartToPolar", select_overload<void(const cv::Mat&, const cv::Mat&, cv::Mat&, cv::Mat&)>(&Wrappers::cartToPolar_wrapper_1));

    function("compare", select_overload<void(const cv::Mat&, const cv::Mat&, cv::Mat&, int)>(&Wrappers::compare_wrapper));

    function("convertScaleAbs", select_overload<void(const cv::Mat&, cv::Mat&, double, double)>(&Wrappers::convertScaleAbs_wrapper));

    function("convertScaleAbs", select_overload<void(const cv::Mat&, cv::Mat&, double)>(&Wrappers::convertScaleAbs_wrapper_1));

    function("convertScaleAbs", select_overload<void(const cv::Mat&, cv::Mat&)>(&Wrappers::convertScaleAbs_wrapper_2));

    function("copyMakeBorder", select_overload<void(const cv::Mat&, cv::Mat&, int, int, int, int, int, const Scalar&)>(&Wrappers::copyMakeBorder_wrapper));

    function("copyMakeBorder", select_overload<void(const cv::Mat&, cv::Mat&, int, int, int, int, int)>(&Wrappers::copyMakeBorder_wrapper_1));

    function("countNonZero", select_overload<int(const cv::Mat&)>(&Wrappers::countNonZero_wrapper));

    function("determinant", select_overload<double(const cv::Mat&)>(&Wrappers::determinant_wrapper));

    function("dft", select_overload<void(const cv::Mat&, cv::Mat&, int, int)>(&Wrappers::dft_wrapper));

    function("dft", select_overload<void(const cv::Mat&, cv::Mat&, int)>(&Wrappers::dft_wrapper_1));

    function("dft", select_overload<void(const cv::Mat&, cv::Mat&)>(&Wrappers::dft_wrapper_2));

    function("divide", select_overload<void(const cv::Mat&, const cv::Mat&, cv::Mat&, double, int)>(&Wrappers::divide_wrapper));

    function("divide", select_overload<void(const cv::Mat&, const cv::Mat&, cv::Mat&, double)>(&Wrappers::divide_wrapper_1));

    function("divide", select_overload<void(const cv::Mat&, const cv::Mat&, cv::Mat&)>(&Wrappers::divide_wrapper_2));

    function("divide1", select_overload<void(double, const cv::Mat&, cv::Mat&, int)>(&Wrappers::divide_wrapper1));

    function("divide1", select_overload<void(double, const cv::Mat&, cv::Mat&)>(&Wrappers::divide_wrapper1_1));

    function("drawFrameAxes", select_overload<void(cv::Mat&, const cv::Mat&, const cv::Mat&, const cv::Mat&, const cv::Mat&, float, int)>(&Wrappers::drawFrameAxes_wrapper));

    function("drawFrameAxes", select_overload<void(cv::Mat&, const cv::Mat&, const cv::Mat&, const cv::Mat&, const cv::Mat&, float)>(&Wrappers::drawFrameAxes_wrapper_1));

    function("eigen", select_overload<bool(const cv::Mat&, cv::Mat&, cv::Mat&)>(&Wrappers::eigen_wrapper));

    function("eigen", select_overload<bool(const cv::Mat&, cv::Mat&)>(&Wrappers::eigen_wrapper_1));

    function("estimateAffine2D", select_overload<Mat(const cv::Mat&, const cv::Mat&, cv::Mat&, int, double, size_t, double, size_t)>(&Wrappers::estimateAffine2D_wrapper));

    function("estimateAffine2D", select_overload<Mat(const cv::Mat&, const cv::Mat&, cv::Mat&, int, double, size_t, double)>(&Wrappers::estimateAffine2D_wrapper_1));

    function("estimateAffine2D", select_overload<Mat(const cv::Mat&, const cv::Mat&, cv::Mat&, int, double, size_t)>(&Wrappers::estimateAffine2D_wrapper_2));

    function("estimateAffine2D", select_overload<Mat(const cv::Mat&, const cv::Mat&, cv::Mat&, int, double)>(&Wrappers::estimateAffine2D_wrapper_3));

    function("estimateAffine2D", select_overload<Mat(const cv::Mat&, const cv::Mat&, cv::Mat&, int)>(&Wrappers::estimateAffine2D_wrapper_4));

    function("estimateAffine2D", select_overload<Mat(const cv::Mat&, const cv::Mat&, cv::Mat&)>(&Wrappers::estimateAffine2D_wrapper_5));

    function("estimateAffine2D", select_overload<Mat(const cv::Mat&, const cv::Mat&)>(&Wrappers::estimateAffine2D_wrapper_6));

    function("estimateAffine2D1", select_overload<Mat(const cv::Mat&, const cv::Mat&, cv::Mat&, const UsacParams&)>(&Wrappers::estimateAffine2D_wrapper1));

    function("exp", select_overload<void(const cv::Mat&, cv::Mat&)>(&Wrappers::exp_wrapper));

    function("findHomography", select_overload<Mat(const cv::Mat&, const cv::Mat&, int, double, cv::Mat&, const int, const double)>(&Wrappers::findHomography_wrapper));

    function("findHomography", select_overload<Mat(const cv::Mat&, const cv::Mat&, int, double, cv::Mat&, const int)>(&Wrappers::findHomography_wrapper_1));

    function("findHomography", select_overload<Mat(const cv::Mat&, const cv::Mat&, int, double, cv::Mat&)>(&Wrappers::findHomography_wrapper_2));

    function("findHomography", select_overload<Mat(const cv::Mat&, const cv::Mat&, int, double)>(&Wrappers::findHomography_wrapper_3));

    function("findHomography", select_overload<Mat(const cv::Mat&, const cv::Mat&, int)>(&Wrappers::findHomography_wrapper_4));

    function("findHomography", select_overload<Mat(const cv::Mat&, const cv::Mat&)>(&Wrappers::findHomography_wrapper_5));

    function("findHomography1", select_overload<Mat(const cv::Mat&, const cv::Mat&, cv::Mat&, const UsacParams&)>(&Wrappers::findHomography_wrapper1));

    function("flip", select_overload<void(const cv::Mat&, cv::Mat&, int)>(&Wrappers::flip_wrapper));

    function("gemm", select_overload<void(const cv::Mat&, const cv::Mat&, double, const cv::Mat&, double, cv::Mat&, int)>(&Wrappers::gemm_wrapper));

    function("gemm", select_overload<void(const cv::Mat&, const cv::Mat&, double, const cv::Mat&, double, cv::Mat&)>(&Wrappers::gemm_wrapper_1));

    function("getDefaultNewCameraMatrix", select_overload<Mat(const cv::Mat&, Size, bool)>(&Wrappers::getDefaultNewCameraMatrix_wrapper));

    function("getDefaultNewCameraMatrix", select_overload<Mat(const cv::Mat&, Size)>(&Wrappers::getDefaultNewCameraMatrix_wrapper_1));

    function("getDefaultNewCameraMatrix", select_overload<Mat(const cv::Mat&)>(&Wrappers::getDefaultNewCameraMatrix_wrapper_2));

    function("getLogLevel", select_overload<int()>(&Wrappers::getLogLevel_wrapper));

    function("getOptimalDFTSize", select_overload<int(int)>(&Wrappers::getOptimalDFTSize_wrapper));

    function("hconcat", select_overload<void(const std::vector<cv::Mat>&, cv::Mat&)>(&Wrappers::hconcat_wrapper));

    function("inRange", select_overload<void(const cv::Mat&, const cv::Mat&, const cv::Mat&, cv::Mat&)>(&Wrappers::inRange_wrapper));

    function("initUndistortRectifyMap", select_overload<void(const cv::Mat&, const cv::Mat&, const cv::Mat&, const cv::Mat&, Size, int, cv::Mat&, cv::Mat&)>(&Wrappers::initUndistortRectifyMap_wrapper));

    function("invert", select_overload<double(const cv::Mat&, cv::Mat&, int)>(&Wrappers::invert_wrapper));

    function("invert", select_overload<double(const cv::Mat&, cv::Mat&)>(&Wrappers::invert_wrapper_1));

    function("kmeans", select_overload<double(const cv::Mat&, int, cv::Mat&, TermCriteria, int, int, cv::Mat&)>(&Wrappers::kmeans_wrapper));

    function("kmeans", select_overload<double(const cv::Mat&, int, cv::Mat&, TermCriteria, int, int)>(&Wrappers::kmeans_wrapper_1));

    function("log", select_overload<void(const cv::Mat&, cv::Mat&)>(&Wrappers::log_wrapper));

    function("magnitude", select_overload<void(const cv::Mat&, const cv::Mat&, cv::Mat&)>(&Wrappers::magnitude_wrapper));

    function("max", select_overload<void(const cv::Mat&, const cv::Mat&, cv::Mat&)>(&Wrappers::max_wrapper));

    function("mean", select_overload<Scalar(const cv::Mat&, const cv::Mat&)>(&Wrappers::mean_wrapper));

    function("mean", select_overload<Scalar(const cv::Mat&)>(&Wrappers::mean_wrapper_1));

    function("meanStdDev", select_overload<void(const cv::Mat&, cv::Mat&, cv::Mat&, const cv::Mat&)>(&Wrappers::meanStdDev_wrapper));

    function("meanStdDev", select_overload<void(const cv::Mat&, cv::Mat&, cv::Mat&)>(&Wrappers::meanStdDev_wrapper_1));

    function("merge", select_overload<void(const std::vector<cv::Mat>&, cv::Mat&)>(&Wrappers::merge_wrapper));

    function("min", select_overload<void(const cv::Mat&, const cv::Mat&, cv::Mat&)>(&Wrappers::min_wrapper));

    function("mixChannels", select_overload<void(const std::vector<cv::Mat>&, std::vector<cv::Mat>&, const emscripten::val&)>(&Wrappers::mixChannels_wrapper));

    function("multiply", select_overload<void(const cv::Mat&, const cv::Mat&, cv::Mat&, double, int)>(&Wrappers::multiply_wrapper));

    function("multiply", select_overload<void(const cv::Mat&, const cv::Mat&, cv::Mat&, double)>(&Wrappers::multiply_wrapper_1));

    function("multiply", select_overload<void(const cv::Mat&, const cv::Mat&, cv::Mat&)>(&Wrappers::multiply_wrapper_2));

    function("norm", select_overload<double(const cv::Mat&, int, const cv::Mat&)>(&Wrappers::norm_wrapper));

    function("norm", select_overload<double(const cv::Mat&, int)>(&Wrappers::norm_wrapper_1));

    function("norm", select_overload<double(const cv::Mat&)>(&Wrappers::norm_wrapper_2));

    function("norm1", select_overload<double(const cv::Mat&, const cv::Mat&, int, const cv::Mat&)>(&Wrappers::norm_wrapper1));

    function("norm1", select_overload<double(const cv::Mat&, const cv::Mat&, int)>(&Wrappers::norm_wrapper1_1));

    function("norm1", select_overload<double(const cv::Mat&, const cv::Mat&)>(&Wrappers::norm_wrapper1_2));

    function("normalize", select_overload<void(const cv::Mat&, cv::Mat&, double, double, int, int, const cv::Mat&)>(&Wrappers::normalize_wrapper));

    function("normalize", select_overload<void(const cv::Mat&, cv::Mat&, double, double, int, int)>(&Wrappers::normalize_wrapper_1));

    function("normalize", select_overload<void(const cv::Mat&, cv::Mat&, double, double, int)>(&Wrappers::normalize_wrapper_2));

    function("normalize", select_overload<void(const cv::Mat&, cv::Mat&, double, double)>(&Wrappers::normalize_wrapper_3));

    function("normalize", select_overload<void(const cv::Mat&, cv::Mat&, double)>(&Wrappers::normalize_wrapper_4));

    function("normalize", select_overload<void(const cv::Mat&, cv::Mat&)>(&Wrappers::normalize_wrapper_5));

    function("perspectiveTransform", select_overload<void(const cv::Mat&, cv::Mat&, const cv::Mat&)>(&Wrappers::perspectiveTransform_wrapper));

    function("polarToCart", select_overload<void(const cv::Mat&, const cv::Mat&, cv::Mat&, cv::Mat&, bool)>(&Wrappers::polarToCart_wrapper));

    function("polarToCart", select_overload<void(const cv::Mat&, const cv::Mat&, cv::Mat&, cv::Mat&)>(&Wrappers::polarToCart_wrapper_1));

    function("pow", select_overload<void(const cv::Mat&, double, cv::Mat&)>(&Wrappers::pow_wrapper));

    function("projectPoints", select_overload<void(const cv::Mat&, const cv::Mat&, const cv::Mat&, const cv::Mat&, const cv::Mat&, cv::Mat&, cv::Mat&, double)>(&Wrappers::projectPoints_wrapper));

    function("projectPoints", select_overload<void(const cv::Mat&, const cv::Mat&, const cv::Mat&, const cv::Mat&, const cv::Mat&, cv::Mat&, cv::Mat&)>(&Wrappers::projectPoints_wrapper_1));

    function("projectPoints", select_overload<void(const cv::Mat&, const cv::Mat&, const cv::Mat&, const cv::Mat&, const cv::Mat&, cv::Mat&)>(&Wrappers::projectPoints_wrapper_2));

    function("randn", select_overload<void(cv::Mat&, const cv::Mat&, const cv::Mat&)>(&Wrappers::randn_wrapper));

    function("randu", select_overload<void(cv::Mat&, const cv::Mat&, const cv::Mat&)>(&Wrappers::randu_wrapper));

    function("reduce", select_overload<void(const cv::Mat&, cv::Mat&, int, int, int)>(&Wrappers::reduce_wrapper));

    function("reduce", select_overload<void(const cv::Mat&, cv::Mat&, int, int)>(&Wrappers::reduce_wrapper_1));

    function("repeat", select_overload<void(const cv::Mat&, int, int, cv::Mat&)>(&Wrappers::repeat_wrapper));

    function("rotate", select_overload<void(const cv::Mat&, cv::Mat&, int)>(&Wrappers::rotate_wrapper));

    function("setIdentity", select_overload<void(cv::Mat&, const Scalar&)>(&Wrappers::setIdentity_wrapper));

    function("setIdentity", select_overload<void(cv::Mat&)>(&Wrappers::setIdentity_wrapper_1));

    function("setLogLevel", select_overload<int(int)>(&Wrappers::setLogLevel_wrapper));

    function("setRNGSeed", select_overload<void(int)>(&Wrappers::setRNGSeed_wrapper));

    function("solve", select_overload<bool(const cv::Mat&, const cv::Mat&, cv::Mat&, int)>(&Wrappers::solve_wrapper));

    function("solve", select_overload<bool(const cv::Mat&, const cv::Mat&, cv::Mat&)>(&Wrappers::solve_wrapper_1));

    function("solvePnP", select_overload<bool(const cv::Mat&, const cv::Mat&, const cv::Mat&, const cv::Mat&, cv::Mat&, cv::Mat&, bool, int)>(&Wrappers::solvePnP_wrapper));

    function("solvePnP", select_overload<bool(const cv::Mat&, const cv::Mat&, const cv::Mat&, const cv::Mat&, cv::Mat&, cv::Mat&, bool)>(&Wrappers::solvePnP_wrapper_1));

    function("solvePnP", select_overload<bool(const cv::Mat&, const cv::Mat&, const cv::Mat&, const cv::Mat&, cv::Mat&, cv::Mat&)>(&Wrappers::solvePnP_wrapper_2));

    function("solvePnPRansac", select_overload<bool(const cv::Mat&, const cv::Mat&, const cv::Mat&, const cv::Mat&, cv::Mat&, cv::Mat&, bool, int, float, double, cv::Mat&, int)>(&Wrappers::solvePnPRansac_wrapper));

    function("solvePnPRansac", select_overload<bool(const cv::Mat&, const cv::Mat&, const cv::Mat&, const cv::Mat&, cv::Mat&, cv::Mat&, bool, int, float, double, cv::Mat&)>(&Wrappers::solvePnPRansac_wrapper_1));

    function("solvePnPRansac", select_overload<bool(const cv::Mat&, const cv::Mat&, const cv::Mat&, const cv::Mat&, cv::Mat&, cv::Mat&, bool, int, float, double)>(&Wrappers::solvePnPRansac_wrapper_2));

    function("solvePnPRansac", select_overload<bool(const cv::Mat&, const cv::Mat&, const cv::Mat&, const cv::Mat&, cv::Mat&, cv::Mat&, bool, int, float)>(&Wrappers::solvePnPRansac_wrapper_3));

    function("solvePnPRansac", select_overload<bool(const cv::Mat&, const cv::Mat&, const cv::Mat&, const cv::Mat&, cv::Mat&, cv::Mat&, bool, int)>(&Wrappers::solvePnPRansac_wrapper_4));

    function("solvePnPRansac", select_overload<bool(const cv::Mat&, const cv::Mat&, const cv::Mat&, const cv::Mat&, cv::Mat&, cv::Mat&, bool)>(&Wrappers::solvePnPRansac_wrapper_5));

    function("solvePnPRansac", select_overload<bool(const cv::Mat&, const cv::Mat&, const cv::Mat&, const cv::Mat&, cv::Mat&, cv::Mat&)>(&Wrappers::solvePnPRansac_wrapper_6));

    function("solvePnPRansac1", select_overload<bool(const cv::Mat&, const cv::Mat&, cv::Mat&, const cv::Mat&, cv::Mat&, cv::Mat&, cv::Mat&, const UsacParams&)>(&Wrappers::solvePnPRansac_wrapper1));

    function("solvePnPRansac1", select_overload<bool(const cv::Mat&, const cv::Mat&, cv::Mat&, const cv::Mat&, cv::Mat&, cv::Mat&, cv::Mat&)>(&Wrappers::solvePnPRansac_wrapper1_1));

    function("solvePnPRefineLM", select_overload<void(const cv::Mat&, const cv::Mat&, const cv::Mat&, const cv::Mat&, cv::Mat&, cv::Mat&, TermCriteria)>(&Wrappers::solvePnPRefineLM_wrapper));

    function("solvePnPRefineLM", select_overload<void(const cv::Mat&, const cv::Mat&, const cv::Mat&, const cv::Mat&, cv::Mat&, cv::Mat&)>(&Wrappers::solvePnPRefineLM_wrapper_1));

    function("solvePoly", select_overload<double(const cv::Mat&, cv::Mat&, int)>(&Wrappers::solvePoly_wrapper));

    function("solvePoly", select_overload<double(const cv::Mat&, cv::Mat&)>(&Wrappers::solvePoly_wrapper_1));

    function("split", select_overload<void(const cv::Mat&, std::vector<cv::Mat>&)>(&Wrappers::split_wrapper));

    function("sqrt", select_overload<void(const cv::Mat&, cv::Mat&)>(&Wrappers::sqrt_wrapper));

    function("subtract", select_overload<void(const cv::Mat&, const cv::Mat&, cv::Mat&, const cv::Mat&, int)>(&Wrappers::subtract_wrapper));

    function("subtract", select_overload<void(const cv::Mat&, const cv::Mat&, cv::Mat&, const cv::Mat&)>(&Wrappers::subtract_wrapper_1));

    function("subtract", select_overload<void(const cv::Mat&, const cv::Mat&, cv::Mat&)>(&Wrappers::subtract_wrapper_2));

    function("trace", select_overload<Scalar(const cv::Mat&)>(&Wrappers::trace_wrapper));

    function("transform", select_overload<void(const cv::Mat&, cv::Mat&, const cv::Mat&)>(&Wrappers::transform_wrapper));

    function("transpose", select_overload<void(const cv::Mat&, cv::Mat&)>(&Wrappers::transpose_wrapper));

    function("undistort", select_overload<void(const cv::Mat&, cv::Mat&, const cv::Mat&, const cv::Mat&, const cv::Mat&)>(&Wrappers::undistort_wrapper));

    function("undistort", select_overload<void(const cv::Mat&, cv::Mat&, const cv::Mat&, const cv::Mat&)>(&Wrappers::undistort_wrapper_1));

    function("vconcat", select_overload<void(const std::vector<cv::Mat>&, cv::Mat&)>(&Wrappers::vconcat_wrapper));

    function("fisheye_initUndistortRectifyMap", select_overload<void(const cv::Mat&, const cv::Mat&, const cv::Mat&, const cv::Mat&, const Size&, int, cv::Mat&, cv::Mat&)>(&Wrappers::fisheye_initUndistortRectifyMap_wrapper));

    function("fisheye_projectPoints", select_overload<void(const cv::Mat&, cv::Mat&, const cv::Mat&, const cv::Mat&, const cv::Mat&, const cv::Mat&, double, cv::Mat&)>(&Wrappers::fisheye_projectPoints_wrapper));

    function("fisheye_projectPoints", select_overload<void(const cv::Mat&, cv::Mat&, const cv::Mat&, const cv::Mat&, const cv::Mat&, const cv::Mat&, double)>(&Wrappers::fisheye_projectPoints_wrapper_1));

    function("fisheye_projectPoints", select_overload<void(const cv::Mat&, cv::Mat&, const cv::Mat&, const cv::Mat&, const cv::Mat&, const cv::Mat&)>(&Wrappers::fisheye_projectPoints_wrapper_2));

    emscripten::class_<cv::Algorithm >("Algorithm");

    constant("ACCESS_FAST", static_cast<long>(cv::ACCESS_FAST));

    constant("ACCESS_MASK", static_cast<long>(cv::ACCESS_MASK));

    constant("ACCESS_READ", static_cast<long>(cv::ACCESS_READ));

    constant("ACCESS_RW", static_cast<long>(cv::ACCESS_RW));

    constant("ACCESS_WRITE", static_cast<long>(cv::ACCESS_WRITE));

    constant("ADAPTIVE_THRESH_GAUSSIAN_C", static_cast<long>(cv::ADAPTIVE_THRESH_GAUSSIAN_C));

    constant("ADAPTIVE_THRESH_MEAN_C", static_cast<long>(cv::ADAPTIVE_THRESH_MEAN_C));

    constant("AKAZE_DESCRIPTOR_KAZE", static_cast<long>(cv::AKAZE::DESCRIPTOR_KAZE));

    constant("AKAZE_DESCRIPTOR_KAZE_UPRIGHT", static_cast<long>(cv::AKAZE::DESCRIPTOR_KAZE_UPRIGHT));

    constant("AKAZE_DESCRIPTOR_MLDB", static_cast<long>(cv::AKAZE::DESCRIPTOR_MLDB));

    constant("AKAZE_DESCRIPTOR_MLDB_UPRIGHT", static_cast<long>(cv::AKAZE::DESCRIPTOR_MLDB_UPRIGHT));

    constant("ALGO_HINT_ACCURATE", static_cast<long>(cv::ALGO_HINT_ACCURATE));

    constant("ALGO_HINT_APPROX", static_cast<long>(cv::ALGO_HINT_APPROX));

    constant("ALGO_HINT_DEFAULT", static_cast<long>(cv::ALGO_HINT_DEFAULT));

    constant("AgastFeatureDetector_AGAST_5_8", static_cast<long>(cv::AgastFeatureDetector::AGAST_5_8));

    constant("AgastFeatureDetector_AGAST_7_12d", static_cast<long>(cv::AgastFeatureDetector::AGAST_7_12d));

    constant("AgastFeatureDetector_AGAST_7_12s", static_cast<long>(cv::AgastFeatureDetector::AGAST_7_12s));

    constant("AgastFeatureDetector_NONMAX_SUPPRESSION", static_cast<long>(cv::AgastFeatureDetector::NONMAX_SUPPRESSION));

    constant("AgastFeatureDetector_OAST_9_16", static_cast<long>(cv::AgastFeatureDetector::OAST_9_16));

    constant("AgastFeatureDetector_THRESHOLD", static_cast<long>(cv::AgastFeatureDetector::THRESHOLD));

    constant("BORDER_CONSTANT", static_cast<long>(cv::BORDER_CONSTANT));

    constant("BORDER_DEFAULT", static_cast<long>(cv::BORDER_DEFAULT));

    constant("BORDER_ISOLATED", static_cast<long>(cv::BORDER_ISOLATED));

    constant("BORDER_REFLECT", static_cast<long>(cv::BORDER_REFLECT));

    constant("BORDER_REFLECT101", static_cast<long>(cv::BORDER_REFLECT101));

    constant("BORDER_REFLECT_101", static_cast<long>(cv::BORDER_REFLECT_101));

    constant("BORDER_REPLICATE", static_cast<long>(cv::BORDER_REPLICATE));

    constant("BORDER_TRANSPARENT", static_cast<long>(cv::BORDER_TRANSPARENT));

    constant("BORDER_WRAP", static_cast<long>(cv::BORDER_WRAP));

    constant("CALIB_CB_ACCURACY", static_cast<long>(cv::CALIB_CB_ACCURACY));

    constant("CALIB_CB_ADAPTIVE_THRESH", static_cast<long>(cv::CALIB_CB_ADAPTIVE_THRESH));

    constant("CALIB_CB_ASYMMETRIC_GRID", static_cast<long>(cv::CALIB_CB_ASYMMETRIC_GRID));

    constant("CALIB_CB_CLUSTERING", static_cast<long>(cv::CALIB_CB_CLUSTERING));

    constant("CALIB_CB_EXHAUSTIVE", static_cast<long>(cv::CALIB_CB_EXHAUSTIVE));

    constant("CALIB_CB_FAST_CHECK", static_cast<long>(cv::CALIB_CB_FAST_CHECK));

    constant("CALIB_CB_FILTER_QUADS", static_cast<long>(cv::CALIB_CB_FILTER_QUADS));

    constant("CALIB_CB_LARGER", static_cast<long>(cv::CALIB_CB_LARGER));

    constant("CALIB_CB_MARKER", static_cast<long>(cv::CALIB_CB_MARKER));

    constant("CALIB_CB_NORMALIZE_IMAGE", static_cast<long>(cv::CALIB_CB_NORMALIZE_IMAGE));

    constant("CALIB_CB_PLAIN", static_cast<long>(cv::CALIB_CB_PLAIN));

    constant("CALIB_CB_SYMMETRIC_GRID", static_cast<long>(cv::CALIB_CB_SYMMETRIC_GRID));

    constant("CALIB_FIX_ASPECT_RATIO", static_cast<long>(cv::CALIB_FIX_ASPECT_RATIO));

    constant("CALIB_FIX_FOCAL_LENGTH", static_cast<long>(cv::CALIB_FIX_FOCAL_LENGTH));

    constant("CALIB_FIX_INTRINSIC", static_cast<long>(cv::CALIB_FIX_INTRINSIC));

    constant("CALIB_FIX_K1", static_cast<long>(cv::CALIB_FIX_K1));

    constant("CALIB_FIX_K2", static_cast<long>(cv::CALIB_FIX_K2));

    constant("CALIB_FIX_K3", static_cast<long>(cv::CALIB_FIX_K3));

    constant("CALIB_FIX_K4", static_cast<long>(cv::CALIB_FIX_K4));

    constant("CALIB_FIX_K5", static_cast<long>(cv::CALIB_FIX_K5));

    constant("CALIB_FIX_K6", static_cast<long>(cv::CALIB_FIX_K6));

    constant("CALIB_FIX_PRINCIPAL_POINT", static_cast<long>(cv::CALIB_FIX_PRINCIPAL_POINT));

    constant("CALIB_FIX_S1_S2_S3_S4", static_cast<long>(cv::CALIB_FIX_S1_S2_S3_S4));

    constant("CALIB_FIX_TANGENT_DIST", static_cast<long>(cv::CALIB_FIX_TANGENT_DIST));

    constant("CALIB_FIX_TAUX_TAUY", static_cast<long>(cv::CALIB_FIX_TAUX_TAUY));

    constant("CALIB_HAND_EYE_ANDREFF", static_cast<long>(cv::CALIB_HAND_EYE_ANDREFF));

    constant("CALIB_HAND_EYE_DANIILIDIS", static_cast<long>(cv::CALIB_HAND_EYE_DANIILIDIS));

    constant("CALIB_HAND_EYE_HORAUD", static_cast<long>(cv::CALIB_HAND_EYE_HORAUD));

    constant("CALIB_HAND_EYE_PARK", static_cast<long>(cv::CALIB_HAND_EYE_PARK));

    constant("CALIB_HAND_EYE_TSAI", static_cast<long>(cv::CALIB_HAND_EYE_TSAI));

    constant("CALIB_NINTRINSIC", static_cast<long>(cv::CALIB_NINTRINSIC));

    constant("CALIB_RATIONAL_MODEL", static_cast<long>(cv::CALIB_RATIONAL_MODEL));

    constant("CALIB_ROBOT_WORLD_HAND_EYE_LI", static_cast<long>(cv::CALIB_ROBOT_WORLD_HAND_EYE_LI));

    constant("CALIB_ROBOT_WORLD_HAND_EYE_SHAH", static_cast<long>(cv::CALIB_ROBOT_WORLD_HAND_EYE_SHAH));

    constant("CALIB_SAME_FOCAL_LENGTH", static_cast<long>(cv::CALIB_SAME_FOCAL_LENGTH));

    constant("CALIB_THIN_PRISM_MODEL", static_cast<long>(cv::CALIB_THIN_PRISM_MODEL));

    constant("CALIB_TILTED_MODEL", static_cast<long>(cv::CALIB_TILTED_MODEL));

    constant("CALIB_USE_EXTRINSIC_GUESS", static_cast<long>(cv::CALIB_USE_EXTRINSIC_GUESS));

    constant("CALIB_USE_INTRINSIC_GUESS", static_cast<long>(cv::CALIB_USE_INTRINSIC_GUESS));

    constant("CALIB_USE_LU", static_cast<long>(cv::CALIB_USE_LU));

    constant("CALIB_USE_QR", static_cast<long>(cv::CALIB_USE_QR));

    constant("CALIB_ZERO_DISPARITY", static_cast<long>(cv::CALIB_ZERO_DISPARITY));

    constant("CALIB_ZERO_TANGENT_DIST", static_cast<long>(cv::CALIB_ZERO_TANGENT_DIST));

    constant("CASCADE_DO_CANNY_PRUNING", static_cast<long>(cv::CASCADE_DO_CANNY_PRUNING));

    constant("CASCADE_DO_ROUGH_SEARCH", static_cast<long>(cv::CASCADE_DO_ROUGH_SEARCH));

    constant("CASCADE_FIND_BIGGEST_OBJECT", static_cast<long>(cv::CASCADE_FIND_BIGGEST_OBJECT));

    constant("CASCADE_SCALE_IMAGE", static_cast<long>(cv::CASCADE_SCALE_IMAGE));

    constant("CCL_BBDT", static_cast<long>(cv::CCL_BBDT));

    constant("CCL_BOLELLI", static_cast<long>(cv::CCL_BOLELLI));

    constant("CCL_DEFAULT", static_cast<long>(cv::CCL_DEFAULT));

    constant("CCL_GRANA", static_cast<long>(cv::CCL_GRANA));

    constant("CCL_SAUF", static_cast<long>(cv::CCL_SAUF));

    constant("CCL_SPAGHETTI", static_cast<long>(cv::CCL_SPAGHETTI));

    constant("CCL_WU", static_cast<long>(cv::CCL_WU));

    constant("CC_STAT_AREA", static_cast<long>(cv::CC_STAT_AREA));

    constant("CC_STAT_HEIGHT", static_cast<long>(cv::CC_STAT_HEIGHT));

    constant("CC_STAT_LEFT", static_cast<long>(cv::CC_STAT_LEFT));

    constant("CC_STAT_MAX", static_cast<long>(cv::CC_STAT_MAX));

    constant("CC_STAT_TOP", static_cast<long>(cv::CC_STAT_TOP));

    constant("CC_STAT_WIDTH", static_cast<long>(cv::CC_STAT_WIDTH));

    constant("CHAIN_APPROX_NONE", static_cast<long>(cv::CHAIN_APPROX_NONE));

    constant("CHAIN_APPROX_SIMPLE", static_cast<long>(cv::CHAIN_APPROX_SIMPLE));

    constant("CHAIN_APPROX_TC89_KCOS", static_cast<long>(cv::CHAIN_APPROX_TC89_KCOS));

    constant("CHAIN_APPROX_TC89_L1", static_cast<long>(cv::CHAIN_APPROX_TC89_L1));

    constant("CMP_EQ", static_cast<long>(cv::CMP_EQ));

    constant("CMP_GE", static_cast<long>(cv::CMP_GE));

    constant("CMP_GT", static_cast<long>(cv::CMP_GT));

    constant("CMP_LE", static_cast<long>(cv::CMP_LE));

    constant("CMP_LT", static_cast<long>(cv::CMP_LT));

    constant("CMP_NE", static_cast<long>(cv::CMP_NE));

    constant("COLORMAP_AUTUMN", static_cast<long>(cv::COLORMAP_AUTUMN));

    constant("COLORMAP_BONE", static_cast<long>(cv::COLORMAP_BONE));

    constant("COLORMAP_CIVIDIS", static_cast<long>(cv::COLORMAP_CIVIDIS));

    constant("COLORMAP_COOL", static_cast<long>(cv::COLORMAP_COOL));

    constant("COLORMAP_DEEPGREEN", static_cast<long>(cv::COLORMAP_DEEPGREEN));

    constant("COLORMAP_HOT", static_cast<long>(cv::COLORMAP_HOT));

    constant("COLORMAP_HSV", static_cast<long>(cv::COLORMAP_HSV));

    constant("COLORMAP_INFERNO", static_cast<long>(cv::COLORMAP_INFERNO));

    constant("COLORMAP_JET", static_cast<long>(cv::COLORMAP_JET));

    constant("COLORMAP_MAGMA", static_cast<long>(cv::COLORMAP_MAGMA));

    constant("COLORMAP_OCEAN", static_cast<long>(cv::COLORMAP_OCEAN));

    constant("COLORMAP_PARULA", static_cast<long>(cv::COLORMAP_PARULA));

    constant("COLORMAP_PINK", static_cast<long>(cv::COLORMAP_PINK));

    constant("COLORMAP_PLASMA", static_cast<long>(cv::COLORMAP_PLASMA));

    constant("COLORMAP_RAINBOW", static_cast<long>(cv::COLORMAP_RAINBOW));

    constant("COLORMAP_SPRING", static_cast<long>(cv::COLORMAP_SPRING));

    constant("COLORMAP_SUMMER", static_cast<long>(cv::COLORMAP_SUMMER));

    constant("COLORMAP_TURBO", static_cast<long>(cv::COLORMAP_TURBO));

    constant("COLORMAP_TWILIGHT", static_cast<long>(cv::COLORMAP_TWILIGHT));

    constant("COLORMAP_TWILIGHT_SHIFTED", static_cast<long>(cv::COLORMAP_TWILIGHT_SHIFTED));

    constant("COLORMAP_VIRIDIS", static_cast<long>(cv::COLORMAP_VIRIDIS));

    constant("COLORMAP_WINTER", static_cast<long>(cv::COLORMAP_WINTER));

    constant("COLOR_BGR2BGR555", static_cast<long>(cv::COLOR_BGR2BGR555));

    constant("COLOR_BGR2BGR565", static_cast<long>(cv::COLOR_BGR2BGR565));

    constant("COLOR_BGR2BGRA", static_cast<long>(cv::COLOR_BGR2BGRA));

    constant("COLOR_BGR2GRAY", static_cast<long>(cv::COLOR_BGR2GRAY));

    constant("COLOR_BGR2HLS", static_cast<long>(cv::COLOR_BGR2HLS));

    constant("COLOR_BGR2HLS_FULL", static_cast<long>(cv::COLOR_BGR2HLS_FULL));

    constant("COLOR_BGR2HSV", static_cast<long>(cv::COLOR_BGR2HSV));

    constant("COLOR_BGR2HSV_FULL", static_cast<long>(cv::COLOR_BGR2HSV_FULL));

    constant("COLOR_BGR2Lab", static_cast<long>(cv::COLOR_BGR2Lab));

    constant("COLOR_BGR2Luv", static_cast<long>(cv::COLOR_BGR2Luv));

    constant("COLOR_BGR2RGB", static_cast<long>(cv::COLOR_BGR2RGB));

    constant("COLOR_BGR2RGBA", static_cast<long>(cv::COLOR_BGR2RGBA));

    constant("COLOR_BGR2XYZ", static_cast<long>(cv::COLOR_BGR2XYZ));

    constant("COLOR_BGR2YCrCb", static_cast<long>(cv::COLOR_BGR2YCrCb));

    constant("COLOR_BGR2YUV", static_cast<long>(cv::COLOR_BGR2YUV));

    constant("COLOR_BGR2YUV_I420", static_cast<long>(cv::COLOR_BGR2YUV_I420));

    constant("COLOR_BGR2YUV_IYUV", static_cast<long>(cv::COLOR_BGR2YUV_IYUV));

    constant("COLOR_BGR2YUV_UYNV", static_cast<long>(cv::COLOR_BGR2YUV_UYNV));

    constant("COLOR_BGR2YUV_UYVY", static_cast<long>(cv::COLOR_BGR2YUV_UYVY));

    constant("COLOR_BGR2YUV_Y422", static_cast<long>(cv::COLOR_BGR2YUV_Y422));

    constant("COLOR_BGR2YUV_YUNV", static_cast<long>(cv::COLOR_BGR2YUV_YUNV));

    constant("COLOR_BGR2YUV_YUY2", static_cast<long>(cv::COLOR_BGR2YUV_YUY2));

    constant("COLOR_BGR2YUV_YUYV", static_cast<long>(cv::COLOR_BGR2YUV_YUYV));

    constant("COLOR_BGR2YUV_YV12", static_cast<long>(cv::COLOR_BGR2YUV_YV12));

    constant("COLOR_BGR2YUV_YVYU", static_cast<long>(cv::COLOR_BGR2YUV_YVYU));

    constant("COLOR_BGR5552BGR", static_cast<long>(cv::COLOR_BGR5552BGR));

    constant("COLOR_BGR5552BGRA", static_cast<long>(cv::COLOR_BGR5552BGRA));

    constant("COLOR_BGR5552GRAY", static_cast<long>(cv::COLOR_BGR5552GRAY));

    constant("COLOR_BGR5552RGB", static_cast<long>(cv::COLOR_BGR5552RGB));

    constant("COLOR_BGR5552RGBA", static_cast<long>(cv::COLOR_BGR5552RGBA));

    constant("COLOR_BGR5652BGR", static_cast<long>(cv::COLOR_BGR5652BGR));

    constant("COLOR_BGR5652BGRA", static_cast<long>(cv::COLOR_BGR5652BGRA));

    constant("COLOR_BGR5652GRAY", static_cast<long>(cv::COLOR_BGR5652GRAY));

    constant("COLOR_BGR5652RGB", static_cast<long>(cv::COLOR_BGR5652RGB));

    constant("COLOR_BGR5652RGBA", static_cast<long>(cv::COLOR_BGR5652RGBA));

    constant("COLOR_BGRA2BGR", static_cast<long>(cv::COLOR_BGRA2BGR));

    constant("COLOR_BGRA2BGR555", static_cast<long>(cv::COLOR_BGRA2BGR555));

    constant("COLOR_BGRA2BGR565", static_cast<long>(cv::COLOR_BGRA2BGR565));

    constant("COLOR_BGRA2GRAY", static_cast<long>(cv::COLOR_BGRA2GRAY));

    constant("COLOR_BGRA2RGB", static_cast<long>(cv::COLOR_BGRA2RGB));

    constant("COLOR_BGRA2RGBA", static_cast<long>(cv::COLOR_BGRA2RGBA));

    constant("COLOR_BGRA2YUV_I420", static_cast<long>(cv::COLOR_BGRA2YUV_I420));

    constant("COLOR_BGRA2YUV_IYUV", static_cast<long>(cv::COLOR_BGRA2YUV_IYUV));

    constant("COLOR_BGRA2YUV_UYNV", static_cast<long>(cv::COLOR_BGRA2YUV_UYNV));

    constant("COLOR_BGRA2YUV_UYVY", static_cast<long>(cv::COLOR_BGRA2YUV_UYVY));

    constant("COLOR_BGRA2YUV_Y422", static_cast<long>(cv::COLOR_BGRA2YUV_Y422));

    constant("COLOR_BGRA2YUV_YUNV", static_cast<long>(cv::COLOR_BGRA2YUV_YUNV));

    constant("COLOR_BGRA2YUV_YUY2", static_cast<long>(cv::COLOR_BGRA2YUV_YUY2));

    constant("COLOR_BGRA2YUV_YUYV", static_cast<long>(cv::COLOR_BGRA2YUV_YUYV));

    constant("COLOR_BGRA2YUV_YV12", static_cast<long>(cv::COLOR_BGRA2YUV_YV12));

    constant("COLOR_BGRA2YUV_YVYU", static_cast<long>(cv::COLOR_BGRA2YUV_YVYU));

    constant("COLOR_BayerBG2BGR", static_cast<long>(cv::COLOR_BayerBG2BGR));

    constant("COLOR_BayerBG2BGRA", static_cast<long>(cv::COLOR_BayerBG2BGRA));

    constant("COLOR_BayerBG2BGR_EA", static_cast<long>(cv::COLOR_BayerBG2BGR_EA));

    constant("COLOR_BayerBG2BGR_VNG", static_cast<long>(cv::COLOR_BayerBG2BGR_VNG));

    constant("COLOR_BayerBG2GRAY", static_cast<long>(cv::COLOR_BayerBG2GRAY));

    constant("COLOR_BayerBG2RGB", static_cast<long>(cv::COLOR_BayerBG2RGB));

    constant("COLOR_BayerBG2RGBA", static_cast<long>(cv::COLOR_BayerBG2RGBA));

    constant("COLOR_BayerBG2RGB_EA", static_cast<long>(cv::COLOR_BayerBG2RGB_EA));

    constant("COLOR_BayerBG2RGB_VNG", static_cast<long>(cv::COLOR_BayerBG2RGB_VNG));

    constant("COLOR_BayerBGGR2BGR", static_cast<long>(cv::COLOR_BayerBGGR2BGR));

    constant("COLOR_BayerBGGR2BGRA", static_cast<long>(cv::COLOR_BayerBGGR2BGRA));

    constant("COLOR_BayerBGGR2BGR_EA", static_cast<long>(cv::COLOR_BayerBGGR2BGR_EA));

    constant("COLOR_BayerBGGR2BGR_VNG", static_cast<long>(cv::COLOR_BayerBGGR2BGR_VNG));

    constant("COLOR_BayerBGGR2GRAY", static_cast<long>(cv::COLOR_BayerBGGR2GRAY));

    constant("COLOR_BayerBGGR2RGB", static_cast<long>(cv::COLOR_BayerBGGR2RGB));

    constant("COLOR_BayerBGGR2RGBA", static_cast<long>(cv::COLOR_BayerBGGR2RGBA));

    constant("COLOR_BayerBGGR2RGB_EA", static_cast<long>(cv::COLOR_BayerBGGR2RGB_EA));

    constant("COLOR_BayerBGGR2RGB_VNG", static_cast<long>(cv::COLOR_BayerBGGR2RGB_VNG));

    constant("COLOR_BayerGB2BGR", static_cast<long>(cv::COLOR_BayerGB2BGR));

    constant("COLOR_BayerGB2BGRA", static_cast<long>(cv::COLOR_BayerGB2BGRA));

    constant("COLOR_BayerGB2BGR_EA", static_cast<long>(cv::COLOR_BayerGB2BGR_EA));

    constant("COLOR_BayerGB2BGR_VNG", static_cast<long>(cv::COLOR_BayerGB2BGR_VNG));

    constant("COLOR_BayerGB2GRAY", static_cast<long>(cv::COLOR_BayerGB2GRAY));

    constant("COLOR_BayerGB2RGB", static_cast<long>(cv::COLOR_BayerGB2RGB));

    constant("COLOR_BayerGB2RGBA", static_cast<long>(cv::COLOR_BayerGB2RGBA));

    constant("COLOR_BayerGB2RGB_EA", static_cast<long>(cv::COLOR_BayerGB2RGB_EA));

    constant("COLOR_BayerGB2RGB_VNG", static_cast<long>(cv::COLOR_BayerGB2RGB_VNG));

    constant("COLOR_BayerGBRG2BGR", static_cast<long>(cv::COLOR_BayerGBRG2BGR));

    constant("COLOR_BayerGBRG2BGRA", static_cast<long>(cv::COLOR_BayerGBRG2BGRA));

    constant("COLOR_BayerGBRG2BGR_EA", static_cast<long>(cv::COLOR_BayerGBRG2BGR_EA));

    constant("COLOR_BayerGBRG2BGR_VNG", static_cast<long>(cv::COLOR_BayerGBRG2BGR_VNG));

    constant("COLOR_BayerGBRG2GRAY", static_cast<long>(cv::COLOR_BayerGBRG2GRAY));

    constant("COLOR_BayerGBRG2RGB", static_cast<long>(cv::COLOR_BayerGBRG2RGB));

    constant("COLOR_BayerGBRG2RGBA", static_cast<long>(cv::COLOR_BayerGBRG2RGBA));

    constant("COLOR_BayerGBRG2RGB_EA", static_cast<long>(cv::COLOR_BayerGBRG2RGB_EA));

    constant("COLOR_BayerGBRG2RGB_VNG", static_cast<long>(cv::COLOR_BayerGBRG2RGB_VNG));

    constant("COLOR_BayerGR2BGR", static_cast<long>(cv::COLOR_BayerGR2BGR));

    constant("COLOR_BayerGR2BGRA", static_cast<long>(cv::COLOR_BayerGR2BGRA));

    constant("COLOR_BayerGR2BGR_EA", static_cast<long>(cv::COLOR_BayerGR2BGR_EA));

    constant("COLOR_BayerGR2BGR_VNG", static_cast<long>(cv::COLOR_BayerGR2BGR_VNG));

    constant("COLOR_BayerGR2GRAY", static_cast<long>(cv::COLOR_BayerGR2GRAY));

    constant("COLOR_BayerGR2RGB", static_cast<long>(cv::COLOR_BayerGR2RGB));

    constant("COLOR_BayerGR2RGBA", static_cast<long>(cv::COLOR_BayerGR2RGBA));

    constant("COLOR_BayerGR2RGB_EA", static_cast<long>(cv::COLOR_BayerGR2RGB_EA));

    constant("COLOR_BayerGR2RGB_VNG", static_cast<long>(cv::COLOR_BayerGR2RGB_VNG));

    constant("COLOR_BayerGRBG2BGR", static_cast<long>(cv::COLOR_BayerGRBG2BGR));

    constant("COLOR_BayerGRBG2BGRA", static_cast<long>(cv::COLOR_BayerGRBG2BGRA));

    constant("COLOR_BayerGRBG2BGR_EA", static_cast<long>(cv::COLOR_BayerGRBG2BGR_EA));

    constant("COLOR_BayerGRBG2BGR_VNG", static_cast<long>(cv::COLOR_BayerGRBG2BGR_VNG));

    constant("COLOR_BayerGRBG2GRAY", static_cast<long>(cv::COLOR_BayerGRBG2GRAY));

    constant("COLOR_BayerGRBG2RGB", static_cast<long>(cv::COLOR_BayerGRBG2RGB));

    constant("COLOR_BayerGRBG2RGBA", static_cast<long>(cv::COLOR_BayerGRBG2RGBA));

    constant("COLOR_BayerGRBG2RGB_EA", static_cast<long>(cv::COLOR_BayerGRBG2RGB_EA));

    constant("COLOR_BayerGRBG2RGB_VNG", static_cast<long>(cv::COLOR_BayerGRBG2RGB_VNG));

    constant("COLOR_BayerRG2BGR", static_cast<long>(cv::COLOR_BayerRG2BGR));

    constant("COLOR_BayerRG2BGRA", static_cast<long>(cv::COLOR_BayerRG2BGRA));

    constant("COLOR_BayerRG2BGR_EA", static_cast<long>(cv::COLOR_BayerRG2BGR_EA));

    constant("COLOR_BayerRG2BGR_VNG", static_cast<long>(cv::COLOR_BayerRG2BGR_VNG));

    constant("COLOR_BayerRG2GRAY", static_cast<long>(cv::COLOR_BayerRG2GRAY));

    constant("COLOR_BayerRG2RGB", static_cast<long>(cv::COLOR_BayerRG2RGB));

    constant("COLOR_BayerRG2RGBA", static_cast<long>(cv::COLOR_BayerRG2RGBA));

    constant("COLOR_BayerRG2RGB_EA", static_cast<long>(cv::COLOR_BayerRG2RGB_EA));

    constant("COLOR_BayerRG2RGB_VNG", static_cast<long>(cv::COLOR_BayerRG2RGB_VNG));

    constant("COLOR_BayerRGGB2BGR", static_cast<long>(cv::COLOR_BayerRGGB2BGR));

    constant("COLOR_BayerRGGB2BGRA", static_cast<long>(cv::COLOR_BayerRGGB2BGRA));

    constant("COLOR_BayerRGGB2BGR_EA", static_cast<long>(cv::COLOR_BayerRGGB2BGR_EA));

    constant("COLOR_BayerRGGB2BGR_VNG", static_cast<long>(cv::COLOR_BayerRGGB2BGR_VNG));

    constant("COLOR_BayerRGGB2GRAY", static_cast<long>(cv::COLOR_BayerRGGB2GRAY));

    constant("COLOR_BayerRGGB2RGB", static_cast<long>(cv::COLOR_BayerRGGB2RGB));

    constant("COLOR_BayerRGGB2RGBA", static_cast<long>(cv::COLOR_BayerRGGB2RGBA));

    constant("COLOR_BayerRGGB2RGB_EA", static_cast<long>(cv::COLOR_BayerRGGB2RGB_EA));

    constant("COLOR_BayerRGGB2RGB_VNG", static_cast<long>(cv::COLOR_BayerRGGB2RGB_VNG));

    constant("COLOR_COLORCVT_MAX", static_cast<long>(cv::COLOR_COLORCVT_MAX));

    constant("COLOR_GRAY2BGR", static_cast<long>(cv::COLOR_GRAY2BGR));

    constant("COLOR_GRAY2BGR555", static_cast<long>(cv::COLOR_GRAY2BGR555));

    constant("COLOR_GRAY2BGR565", static_cast<long>(cv::COLOR_GRAY2BGR565));

    constant("COLOR_GRAY2BGRA", static_cast<long>(cv::COLOR_GRAY2BGRA));

    constant("COLOR_GRAY2RGB", static_cast<long>(cv::COLOR_GRAY2RGB));

    constant("COLOR_GRAY2RGBA", static_cast<long>(cv::COLOR_GRAY2RGBA));

    constant("COLOR_HLS2BGR", static_cast<long>(cv::COLOR_HLS2BGR));

    constant("COLOR_HLS2BGR_FULL", static_cast<long>(cv::COLOR_HLS2BGR_FULL));

    constant("COLOR_HLS2RGB", static_cast<long>(cv::COLOR_HLS2RGB));

    constant("COLOR_HLS2RGB_FULL", static_cast<long>(cv::COLOR_HLS2RGB_FULL));

    constant("COLOR_HSV2BGR", static_cast<long>(cv::COLOR_HSV2BGR));

    constant("COLOR_HSV2BGR_FULL", static_cast<long>(cv::COLOR_HSV2BGR_FULL));

    constant("COLOR_HSV2RGB", static_cast<long>(cv::COLOR_HSV2RGB));

    constant("COLOR_HSV2RGB_FULL", static_cast<long>(cv::COLOR_HSV2RGB_FULL));

    constant("COLOR_LBGR2Lab", static_cast<long>(cv::COLOR_LBGR2Lab));

    constant("COLOR_LBGR2Luv", static_cast<long>(cv::COLOR_LBGR2Luv));

    constant("COLOR_LRGB2Lab", static_cast<long>(cv::COLOR_LRGB2Lab));

    constant("COLOR_LRGB2Luv", static_cast<long>(cv::COLOR_LRGB2Luv));

    constant("COLOR_Lab2BGR", static_cast<long>(cv::COLOR_Lab2BGR));

    constant("COLOR_Lab2LBGR", static_cast<long>(cv::COLOR_Lab2LBGR));

    constant("COLOR_Lab2LRGB", static_cast<long>(cv::COLOR_Lab2LRGB));

    constant("COLOR_Lab2RGB", static_cast<long>(cv::COLOR_Lab2RGB));

    constant("COLOR_Luv2BGR", static_cast<long>(cv::COLOR_Luv2BGR));

    constant("COLOR_Luv2LBGR", static_cast<long>(cv::COLOR_Luv2LBGR));

    constant("COLOR_Luv2LRGB", static_cast<long>(cv::COLOR_Luv2LRGB));

    constant("COLOR_Luv2RGB", static_cast<long>(cv::COLOR_Luv2RGB));

    constant("COLOR_RGB2BGR", static_cast<long>(cv::COLOR_RGB2BGR));

    constant("COLOR_RGB2BGR555", static_cast<long>(cv::COLOR_RGB2BGR555));

    constant("COLOR_RGB2BGR565", static_cast<long>(cv::COLOR_RGB2BGR565));

    constant("COLOR_RGB2BGRA", static_cast<long>(cv::COLOR_RGB2BGRA));

    constant("COLOR_RGB2GRAY", static_cast<long>(cv::COLOR_RGB2GRAY));

    constant("COLOR_RGB2HLS", static_cast<long>(cv::COLOR_RGB2HLS));

    constant("COLOR_RGB2HLS_FULL", static_cast<long>(cv::COLOR_RGB2HLS_FULL));

    constant("COLOR_RGB2HSV", static_cast<long>(cv::COLOR_RGB2HSV));

    constant("COLOR_RGB2HSV_FULL", static_cast<long>(cv::COLOR_RGB2HSV_FULL));

    constant("COLOR_RGB2Lab", static_cast<long>(cv::COLOR_RGB2Lab));

    constant("COLOR_RGB2Luv", static_cast<long>(cv::COLOR_RGB2Luv));

    constant("COLOR_RGB2RGBA", static_cast<long>(cv::COLOR_RGB2RGBA));

    constant("COLOR_RGB2XYZ", static_cast<long>(cv::COLOR_RGB2XYZ));

    constant("COLOR_RGB2YCrCb", static_cast<long>(cv::COLOR_RGB2YCrCb));

    constant("COLOR_RGB2YUV", static_cast<long>(cv::COLOR_RGB2YUV));

    constant("COLOR_RGB2YUV_I420", static_cast<long>(cv::COLOR_RGB2YUV_I420));

    constant("COLOR_RGB2YUV_IYUV", static_cast<long>(cv::COLOR_RGB2YUV_IYUV));

    constant("COLOR_RGB2YUV_UYNV", static_cast<long>(cv::COLOR_RGB2YUV_UYNV));

    constant("COLOR_RGB2YUV_UYVY", static_cast<long>(cv::COLOR_RGB2YUV_UYVY));

    constant("COLOR_RGB2YUV_Y422", static_cast<long>(cv::COLOR_RGB2YUV_Y422));

    constant("COLOR_RGB2YUV_YUNV", static_cast<long>(cv::COLOR_RGB2YUV_YUNV));

    constant("COLOR_RGB2YUV_YUY2", static_cast<long>(cv::COLOR_RGB2YUV_YUY2));

    constant("COLOR_RGB2YUV_YUYV", static_cast<long>(cv::COLOR_RGB2YUV_YUYV));

    constant("COLOR_RGB2YUV_YV12", static_cast<long>(cv::COLOR_RGB2YUV_YV12));

    constant("COLOR_RGB2YUV_YVYU", static_cast<long>(cv::COLOR_RGB2YUV_YVYU));

    constant("COLOR_RGBA2BGR", static_cast<long>(cv::COLOR_RGBA2BGR));

    constant("COLOR_RGBA2BGR555", static_cast<long>(cv::COLOR_RGBA2BGR555));

    constant("COLOR_RGBA2BGR565", static_cast<long>(cv::COLOR_RGBA2BGR565));

    constant("COLOR_RGBA2BGRA", static_cast<long>(cv::COLOR_RGBA2BGRA));

    constant("COLOR_RGBA2GRAY", static_cast<long>(cv::COLOR_RGBA2GRAY));

    constant("COLOR_RGBA2RGB", static_cast<long>(cv::COLOR_RGBA2RGB));

    constant("COLOR_RGBA2YUV_I420", static_cast<long>(cv::COLOR_RGBA2YUV_I420));

    constant("COLOR_RGBA2YUV_IYUV", static_cast<long>(cv::COLOR_RGBA2YUV_IYUV));

    constant("COLOR_RGBA2YUV_UYNV", static_cast<long>(cv::COLOR_RGBA2YUV_UYNV));

    constant("COLOR_RGBA2YUV_UYVY", static_cast<long>(cv::COLOR_RGBA2YUV_UYVY));

    constant("COLOR_RGBA2YUV_Y422", static_cast<long>(cv::COLOR_RGBA2YUV_Y422));

    constant("COLOR_RGBA2YUV_YUNV", static_cast<long>(cv::COLOR_RGBA2YUV_YUNV));

    constant("COLOR_RGBA2YUV_YUY2", static_cast<long>(cv::COLOR_RGBA2YUV_YUY2));

    constant("COLOR_RGBA2YUV_YUYV", static_cast<long>(cv::COLOR_RGBA2YUV_YUYV));

    constant("COLOR_RGBA2YUV_YV12", static_cast<long>(cv::COLOR_RGBA2YUV_YV12));

    constant("COLOR_RGBA2YUV_YVYU", static_cast<long>(cv::COLOR_RGBA2YUV_YVYU));

    constant("COLOR_RGBA2mRGBA", static_cast<long>(cv::COLOR_RGBA2mRGBA));

    constant("COLOR_XYZ2BGR", static_cast<long>(cv::COLOR_XYZ2BGR));

    constant("COLOR_XYZ2RGB", static_cast<long>(cv::COLOR_XYZ2RGB));

    constant("COLOR_YCrCb2BGR", static_cast<long>(cv::COLOR_YCrCb2BGR));

    constant("COLOR_YCrCb2RGB", static_cast<long>(cv::COLOR_YCrCb2RGB));

    constant("COLOR_YUV2BGR", static_cast<long>(cv::COLOR_YUV2BGR));

    constant("COLOR_YUV2BGRA_I420", static_cast<long>(cv::COLOR_YUV2BGRA_I420));

    constant("COLOR_YUV2BGRA_IYUV", static_cast<long>(cv::COLOR_YUV2BGRA_IYUV));

    constant("COLOR_YUV2BGRA_NV12", static_cast<long>(cv::COLOR_YUV2BGRA_NV12));

    constant("COLOR_YUV2BGRA_NV21", static_cast<long>(cv::COLOR_YUV2BGRA_NV21));

    constant("COLOR_YUV2BGRA_UYNV", static_cast<long>(cv::COLOR_YUV2BGRA_UYNV));

    constant("COLOR_YUV2BGRA_UYVY", static_cast<long>(cv::COLOR_YUV2BGRA_UYVY));

    constant("COLOR_YUV2BGRA_Y422", static_cast<long>(cv::COLOR_YUV2BGRA_Y422));

    constant("COLOR_YUV2BGRA_YUNV", static_cast<long>(cv::COLOR_YUV2BGRA_YUNV));

    constant("COLOR_YUV2BGRA_YUY2", static_cast<long>(cv::COLOR_YUV2BGRA_YUY2));

    constant("COLOR_YUV2BGRA_YUYV", static_cast<long>(cv::COLOR_YUV2BGRA_YUYV));

    constant("COLOR_YUV2BGRA_YV12", static_cast<long>(cv::COLOR_YUV2BGRA_YV12));

    constant("COLOR_YUV2BGRA_YVYU", static_cast<long>(cv::COLOR_YUV2BGRA_YVYU));

    constant("COLOR_YUV2BGR_I420", static_cast<long>(cv::COLOR_YUV2BGR_I420));

    constant("COLOR_YUV2BGR_IYUV", static_cast<long>(cv::COLOR_YUV2BGR_IYUV));

    constant("COLOR_YUV2BGR_NV12", static_cast<long>(cv::COLOR_YUV2BGR_NV12));

    constant("COLOR_YUV2BGR_NV21", static_cast<long>(cv::COLOR_YUV2BGR_NV21));

    constant("COLOR_YUV2BGR_UYNV", static_cast<long>(cv::COLOR_YUV2BGR_UYNV));

    constant("COLOR_YUV2BGR_UYVY", static_cast<long>(cv::COLOR_YUV2BGR_UYVY));

    constant("COLOR_YUV2BGR_Y422", static_cast<long>(cv::COLOR_YUV2BGR_Y422));

    constant("COLOR_YUV2BGR_YUNV", static_cast<long>(cv::COLOR_YUV2BGR_YUNV));

    constant("COLOR_YUV2BGR_YUY2", static_cast<long>(cv::COLOR_YUV2BGR_YUY2));

    constant("COLOR_YUV2BGR_YUYV", static_cast<long>(cv::COLOR_YUV2BGR_YUYV));

    constant("COLOR_YUV2BGR_YV12", static_cast<long>(cv::COLOR_YUV2BGR_YV12));

    constant("COLOR_YUV2BGR_YVYU", static_cast<long>(cv::COLOR_YUV2BGR_YVYU));

    constant("COLOR_YUV2GRAY_420", static_cast<long>(cv::COLOR_YUV2GRAY_420));

    constant("COLOR_YUV2GRAY_I420", static_cast<long>(cv::COLOR_YUV2GRAY_I420));

    constant("COLOR_YUV2GRAY_IYUV", static_cast<long>(cv::COLOR_YUV2GRAY_IYUV));

    constant("COLOR_YUV2GRAY_NV12", static_cast<long>(cv::COLOR_YUV2GRAY_NV12));

    constant("COLOR_YUV2GRAY_NV21", static_cast<long>(cv::COLOR_YUV2GRAY_NV21));

    constant("COLOR_YUV2GRAY_UYNV", static_cast<long>(cv::COLOR_YUV2GRAY_UYNV));

    constant("COLOR_YUV2GRAY_UYVY", static_cast<long>(cv::COLOR_YUV2GRAY_UYVY));

    constant("COLOR_YUV2GRAY_Y422", static_cast<long>(cv::COLOR_YUV2GRAY_Y422));

    constant("COLOR_YUV2GRAY_YUNV", static_cast<long>(cv::COLOR_YUV2GRAY_YUNV));

    constant("COLOR_YUV2GRAY_YUY2", static_cast<long>(cv::COLOR_YUV2GRAY_YUY2));

    constant("COLOR_YUV2GRAY_YUYV", static_cast<long>(cv::COLOR_YUV2GRAY_YUYV));

    constant("COLOR_YUV2GRAY_YV12", static_cast<long>(cv::COLOR_YUV2GRAY_YV12));

    constant("COLOR_YUV2GRAY_YVYU", static_cast<long>(cv::COLOR_YUV2GRAY_YVYU));

    constant("COLOR_YUV2RGB", static_cast<long>(cv::COLOR_YUV2RGB));

    constant("COLOR_YUV2RGBA_I420", static_cast<long>(cv::COLOR_YUV2RGBA_I420));

    constant("COLOR_YUV2RGBA_IYUV", static_cast<long>(cv::COLOR_YUV2RGBA_IYUV));

    constant("COLOR_YUV2RGBA_NV12", static_cast<long>(cv::COLOR_YUV2RGBA_NV12));

    constant("COLOR_YUV2RGBA_NV21", static_cast<long>(cv::COLOR_YUV2RGBA_NV21));

    constant("COLOR_YUV2RGBA_UYNV", static_cast<long>(cv::COLOR_YUV2RGBA_UYNV));

    constant("COLOR_YUV2RGBA_UYVY", static_cast<long>(cv::COLOR_YUV2RGBA_UYVY));

    constant("COLOR_YUV2RGBA_Y422", static_cast<long>(cv::COLOR_YUV2RGBA_Y422));

    constant("COLOR_YUV2RGBA_YUNV", static_cast<long>(cv::COLOR_YUV2RGBA_YUNV));

    constant("COLOR_YUV2RGBA_YUY2", static_cast<long>(cv::COLOR_YUV2RGBA_YUY2));

    constant("COLOR_YUV2RGBA_YUYV", static_cast<long>(cv::COLOR_YUV2RGBA_YUYV));

    constant("COLOR_YUV2RGBA_YV12", static_cast<long>(cv::COLOR_YUV2RGBA_YV12));

    constant("COLOR_YUV2RGBA_YVYU", static_cast<long>(cv::COLOR_YUV2RGBA_YVYU));

    constant("COLOR_YUV2RGB_I420", static_cast<long>(cv::COLOR_YUV2RGB_I420));

    constant("COLOR_YUV2RGB_IYUV", static_cast<long>(cv::COLOR_YUV2RGB_IYUV));

    constant("COLOR_YUV2RGB_NV12", static_cast<long>(cv::COLOR_YUV2RGB_NV12));

    constant("COLOR_YUV2RGB_NV21", static_cast<long>(cv::COLOR_YUV2RGB_NV21));

    constant("COLOR_YUV2RGB_UYNV", static_cast<long>(cv::COLOR_YUV2RGB_UYNV));

    constant("COLOR_YUV2RGB_UYVY", static_cast<long>(cv::COLOR_YUV2RGB_UYVY));

    constant("COLOR_YUV2RGB_Y422", static_cast<long>(cv::COLOR_YUV2RGB_Y422));

    constant("COLOR_YUV2RGB_YUNV", static_cast<long>(cv::COLOR_YUV2RGB_YUNV));

    constant("COLOR_YUV2RGB_YUY2", static_cast<long>(cv::COLOR_YUV2RGB_YUY2));

    constant("COLOR_YUV2RGB_YUYV", static_cast<long>(cv::COLOR_YUV2RGB_YUYV));

    constant("COLOR_YUV2RGB_YV12", static_cast<long>(cv::COLOR_YUV2RGB_YV12));

    constant("COLOR_YUV2RGB_YVYU", static_cast<long>(cv::COLOR_YUV2RGB_YVYU));

    constant("COLOR_YUV420p2BGR", static_cast<long>(cv::COLOR_YUV420p2BGR));

    constant("COLOR_YUV420p2BGRA", static_cast<long>(cv::COLOR_YUV420p2BGRA));

    constant("COLOR_YUV420p2GRAY", static_cast<long>(cv::COLOR_YUV420p2GRAY));

    constant("COLOR_YUV420p2RGB", static_cast<long>(cv::COLOR_YUV420p2RGB));

    constant("COLOR_YUV420p2RGBA", static_cast<long>(cv::COLOR_YUV420p2RGBA));

    constant("COLOR_YUV420sp2BGR", static_cast<long>(cv::COLOR_YUV420sp2BGR));

    constant("COLOR_YUV420sp2BGRA", static_cast<long>(cv::COLOR_YUV420sp2BGRA));

    constant("COLOR_YUV420sp2GRAY", static_cast<long>(cv::COLOR_YUV420sp2GRAY));

    constant("COLOR_YUV420sp2RGB", static_cast<long>(cv::COLOR_YUV420sp2RGB));

    constant("COLOR_YUV420sp2RGBA", static_cast<long>(cv::COLOR_YUV420sp2RGBA));

    constant("COLOR_mRGBA2RGBA", static_cast<long>(cv::COLOR_mRGBA2RGBA));

    constant("CONTOURS_MATCH_I1", static_cast<long>(cv::CONTOURS_MATCH_I1));

    constant("CONTOURS_MATCH_I2", static_cast<long>(cv::CONTOURS_MATCH_I2));

    constant("CONTOURS_MATCH_I3", static_cast<long>(cv::CONTOURS_MATCH_I3));

    constant("COVAR_COLS", static_cast<long>(cv::COVAR_COLS));

    constant("COVAR_NORMAL", static_cast<long>(cv::COVAR_NORMAL));

    constant("COVAR_ROWS", static_cast<long>(cv::COVAR_ROWS));

    constant("COVAR_SCALE", static_cast<long>(cv::COVAR_SCALE));

    constant("COVAR_SCRAMBLED", static_cast<long>(cv::COVAR_SCRAMBLED));

    constant("COVAR_USE_AVG", static_cast<long>(cv::COVAR_USE_AVG));

    constant("COV_POLISHER", static_cast<long>(cv::COV_POLISHER));

    constant("CirclesGridFinderParameters_ASYMMETRIC_GRID", static_cast<long>(cv::CirclesGridFinderParameters::ASYMMETRIC_GRID));

    constant("CirclesGridFinderParameters_SYMMETRIC_GRID", static_cast<long>(cv::CirclesGridFinderParameters::SYMMETRIC_GRID));

    constant("DCT_INVERSE", static_cast<long>(cv::DCT_INVERSE));

    constant("DCT_ROWS", static_cast<long>(cv::DCT_ROWS));

    constant("DECOMP_CHOLESKY", static_cast<long>(cv::DECOMP_CHOLESKY));

    constant("DECOMP_EIG", static_cast<long>(cv::DECOMP_EIG));

    constant("DECOMP_LU", static_cast<long>(cv::DECOMP_LU));

    constant("DECOMP_NORMAL", static_cast<long>(cv::DECOMP_NORMAL));

    constant("DECOMP_QR", static_cast<long>(cv::DECOMP_QR));

    constant("DECOMP_SVD", static_cast<long>(cv::DECOMP_SVD));

    constant("DFT_COMPLEX_INPUT", static_cast<long>(cv::DFT_COMPLEX_INPUT));

    constant("DFT_COMPLEX_OUTPUT", static_cast<long>(cv::DFT_COMPLEX_OUTPUT));

    constant("DFT_INVERSE", static_cast<long>(cv::DFT_INVERSE));

    constant("DFT_REAL_OUTPUT", static_cast<long>(cv::DFT_REAL_OUTPUT));

    constant("DFT_ROWS", static_cast<long>(cv::DFT_ROWS));

    constant("DFT_SCALE", static_cast<long>(cv::DFT_SCALE));

    constant("DISOpticalFlow_PRESET_FAST", static_cast<long>(cv::DISOpticalFlow::PRESET_FAST));

    constant("DISOpticalFlow_PRESET_MEDIUM", static_cast<long>(cv::DISOpticalFlow::PRESET_MEDIUM));

    constant("DISOpticalFlow_PRESET_ULTRAFAST", static_cast<long>(cv::DISOpticalFlow::PRESET_ULTRAFAST));

    constant("DIST_C", static_cast<long>(cv::DIST_C));

    constant("DIST_FAIR", static_cast<long>(cv::DIST_FAIR));

    constant("DIST_HUBER", static_cast<long>(cv::DIST_HUBER));

    constant("DIST_L1", static_cast<long>(cv::DIST_L1));

    constant("DIST_L12", static_cast<long>(cv::DIST_L12));

    constant("DIST_L2", static_cast<long>(cv::DIST_L2));

    constant("DIST_LABEL_CCOMP", static_cast<long>(cv::DIST_LABEL_CCOMP));

    constant("DIST_LABEL_PIXEL", static_cast<long>(cv::DIST_LABEL_PIXEL));

    constant("DIST_MASK_3", static_cast<long>(cv::DIST_MASK_3));

    constant("DIST_MASK_5", static_cast<long>(cv::DIST_MASK_5));

    constant("DIST_MASK_PRECISE", static_cast<long>(cv::DIST_MASK_PRECISE));

    constant("DIST_USER", static_cast<long>(cv::DIST_USER));

    constant("DIST_WELSCH", static_cast<long>(cv::DIST_WELSCH));

    constant("DescriptorMatcher_BRUTEFORCE", static_cast<long>(cv::DescriptorMatcher::BRUTEFORCE));

    constant("DescriptorMatcher_BRUTEFORCE_HAMMING", static_cast<long>(cv::DescriptorMatcher::BRUTEFORCE_HAMMING));

    constant("DescriptorMatcher_BRUTEFORCE_HAMMINGLUT", static_cast<long>(cv::DescriptorMatcher::BRUTEFORCE_HAMMINGLUT));

    constant("DescriptorMatcher_BRUTEFORCE_L1", static_cast<long>(cv::DescriptorMatcher::BRUTEFORCE_L1));

    constant("DescriptorMatcher_BRUTEFORCE_SL2", static_cast<long>(cv::DescriptorMatcher::BRUTEFORCE_SL2));

    constant("DescriptorMatcher_FLANNBASED", static_cast<long>(cv::DescriptorMatcher::FLANNBASED));

    constant("DrawMatchesFlags_DEFAULT", static_cast<long>(cv::DrawMatchesFlags::DEFAULT));

    constant("DrawMatchesFlags_DRAW_OVER_OUTIMG", static_cast<long>(cv::DrawMatchesFlags::DRAW_OVER_OUTIMG));

    constant("DrawMatchesFlags_DRAW_RICH_KEYPOINTS", static_cast<long>(cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS));

    constant("DrawMatchesFlags_NOT_DRAW_SINGLE_POINTS", static_cast<long>(cv::DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS));

    constant("FILLED", static_cast<long>(cv::FILLED));

    constant("FILTER_SCHARR", static_cast<long>(cv::FILTER_SCHARR));

    constant("FLOODFILL_FIXED_RANGE", static_cast<long>(cv::FLOODFILL_FIXED_RANGE));

    constant("FLOODFILL_MASK_ONLY", static_cast<long>(cv::FLOODFILL_MASK_ONLY));

    constant("FM_7POINT", static_cast<long>(cv::FM_7POINT));

    constant("FM_8POINT", static_cast<long>(cv::FM_8POINT));

    constant("FM_LMEDS", static_cast<long>(cv::FM_LMEDS));

    constant("FM_RANSAC", static_cast<long>(cv::FM_RANSAC));

    constant("FONT_HERSHEY_COMPLEX", static_cast<long>(cv::FONT_HERSHEY_COMPLEX));

    constant("FONT_HERSHEY_COMPLEX_SMALL", static_cast<long>(cv::FONT_HERSHEY_COMPLEX_SMALL));

    constant("FONT_HERSHEY_DUPLEX", static_cast<long>(cv::FONT_HERSHEY_DUPLEX));

    constant("FONT_HERSHEY_PLAIN", static_cast<long>(cv::FONT_HERSHEY_PLAIN));

    constant("FONT_HERSHEY_SCRIPT_COMPLEX", static_cast<long>(cv::FONT_HERSHEY_SCRIPT_COMPLEX));

    constant("FONT_HERSHEY_SCRIPT_SIMPLEX", static_cast<long>(cv::FONT_HERSHEY_SCRIPT_SIMPLEX));

    constant("FONT_HERSHEY_SIMPLEX", static_cast<long>(cv::FONT_HERSHEY_SIMPLEX));

    constant("FONT_HERSHEY_TRIPLEX", static_cast<long>(cv::FONT_HERSHEY_TRIPLEX));

    constant("FONT_ITALIC", static_cast<long>(cv::FONT_ITALIC));

    constant("FaceRecognizerSF_FR_COSINE", static_cast<long>(cv::FaceRecognizerSF::FR_COSINE));

    constant("FaceRecognizerSF_FR_NORM_L2", static_cast<long>(cv::FaceRecognizerSF::FR_NORM_L2));

    constant("FastFeatureDetector_FAST_N", static_cast<long>(cv::FastFeatureDetector::FAST_N));

    constant("FastFeatureDetector_NONMAX_SUPPRESSION", static_cast<long>(cv::FastFeatureDetector::NONMAX_SUPPRESSION));

    constant("FastFeatureDetector_THRESHOLD", static_cast<long>(cv::FastFeatureDetector::THRESHOLD));

    constant("FastFeatureDetector_TYPE_5_8", static_cast<long>(cv::FastFeatureDetector::TYPE_5_8));

    constant("FastFeatureDetector_TYPE_7_12", static_cast<long>(cv::FastFeatureDetector::TYPE_7_12));

    constant("FastFeatureDetector_TYPE_9_16", static_cast<long>(cv::FastFeatureDetector::TYPE_9_16));

    constant("FileNode_EMPTY", static_cast<long>(cv::FileNode::EMPTY));

    constant("FileNode_FLOAT", static_cast<long>(cv::FileNode::FLOAT));

    constant("FileNode_FLOW", static_cast<long>(cv::FileNode::FLOW));

    constant("FileNode_INT", static_cast<long>(cv::FileNode::INT));

    constant("FileNode_MAP", static_cast<long>(cv::FileNode::MAP));

    constant("FileNode_NAMED", static_cast<long>(cv::FileNode::NAMED));

    constant("FileNode_NONE", static_cast<long>(cv::FileNode::NONE));

    constant("FileNode_REAL", static_cast<long>(cv::FileNode::REAL));

    constant("FileNode_SEQ", static_cast<long>(cv::FileNode::SEQ));

    constant("FileNode_STR", static_cast<long>(cv::FileNode::STR));

    constant("FileNode_STRING", static_cast<long>(cv::FileNode::STRING));

    constant("FileNode_TYPE_MASK", static_cast<long>(cv::FileNode::TYPE_MASK));

    constant("FileNode_UNIFORM", static_cast<long>(cv::FileNode::UNIFORM));

    constant("FileStorage_APPEND", static_cast<long>(cv::FileStorage::APPEND));

    constant("FileStorage_BASE64", static_cast<long>(cv::FileStorage::BASE64));

    constant("FileStorage_FORMAT_AUTO", static_cast<long>(cv::FileStorage::FORMAT_AUTO));

    constant("FileStorage_FORMAT_JSON", static_cast<long>(cv::FileStorage::FORMAT_JSON));

    constant("FileStorage_FORMAT_MASK", static_cast<long>(cv::FileStorage::FORMAT_MASK));

    constant("FileStorage_FORMAT_XML", static_cast<long>(cv::FileStorage::FORMAT_XML));

    constant("FileStorage_FORMAT_YAML", static_cast<long>(cv::FileStorage::FORMAT_YAML));

    constant("FileStorage_INSIDE_MAP", static_cast<long>(cv::FileStorage::INSIDE_MAP));

    constant("FileStorage_MEMORY", static_cast<long>(cv::FileStorage::MEMORY));

    constant("FileStorage_NAME_EXPECTED", static_cast<long>(cv::FileStorage::NAME_EXPECTED));

    constant("FileStorage_READ", static_cast<long>(cv::FileStorage::READ));

    constant("FileStorage_UNDEFINED", static_cast<long>(cv::FileStorage::UNDEFINED));

    constant("FileStorage_VALUE_EXPECTED", static_cast<long>(cv::FileStorage::VALUE_EXPECTED));

    constant("FileStorage_WRITE", static_cast<long>(cv::FileStorage::WRITE));

    constant("FileStorage_WRITE_BASE64", static_cast<long>(cv::FileStorage::WRITE_BASE64));

    constant("Formatter_FMT_C", static_cast<long>(cv::Formatter::FMT_C));

    constant("Formatter_FMT_CSV", static_cast<long>(cv::Formatter::FMT_CSV));

    constant("Formatter_FMT_DEFAULT", static_cast<long>(cv::Formatter::FMT_DEFAULT));

    constant("Formatter_FMT_MATLAB", static_cast<long>(cv::Formatter::FMT_MATLAB));

    constant("Formatter_FMT_NUMPY", static_cast<long>(cv::Formatter::FMT_NUMPY));

    constant("Formatter_FMT_PYTHON", static_cast<long>(cv::Formatter::FMT_PYTHON));

    constant("GC_BGD", static_cast<long>(cv::GC_BGD));

    constant("GC_EVAL", static_cast<long>(cv::GC_EVAL));

    constant("GC_EVAL_FREEZE_MODEL", static_cast<long>(cv::GC_EVAL_FREEZE_MODEL));

    constant("GC_FGD", static_cast<long>(cv::GC_FGD));

    constant("GC_INIT_WITH_MASK", static_cast<long>(cv::GC_INIT_WITH_MASK));

    constant("GC_INIT_WITH_RECT", static_cast<long>(cv::GC_INIT_WITH_RECT));

    constant("GC_PR_BGD", static_cast<long>(cv::GC_PR_BGD));

    constant("GC_PR_FGD", static_cast<long>(cv::GC_PR_FGD));

    constant("GEMM_1_T", static_cast<long>(cv::GEMM_1_T));

    constant("GEMM_2_T", static_cast<long>(cv::GEMM_2_T));

    constant("GEMM_3_T", static_cast<long>(cv::GEMM_3_T));

    constant("HISTCMP_BHATTACHARYYA", static_cast<long>(cv::HISTCMP_BHATTACHARYYA));

    constant("HISTCMP_CHISQR", static_cast<long>(cv::HISTCMP_CHISQR));

    constant("HISTCMP_CHISQR_ALT", static_cast<long>(cv::HISTCMP_CHISQR_ALT));

    constant("HISTCMP_CORREL", static_cast<long>(cv::HISTCMP_CORREL));

    constant("HISTCMP_HELLINGER", static_cast<long>(cv::HISTCMP_HELLINGER));

    constant("HISTCMP_INTERSECT", static_cast<long>(cv::HISTCMP_INTERSECT));

    constant("HISTCMP_KL_DIV", static_cast<long>(cv::HISTCMP_KL_DIV));

    constant("HOGDescriptor_DEFAULT_NLEVELS", static_cast<long>(cv::HOGDescriptor::DEFAULT_NLEVELS));

    constant("HOGDescriptor_DESCR_FORMAT_COL_BY_COL", static_cast<long>(cv::HOGDescriptor::DESCR_FORMAT_COL_BY_COL));

    constant("HOGDescriptor_DESCR_FORMAT_ROW_BY_ROW", static_cast<long>(cv::HOGDescriptor::DESCR_FORMAT_ROW_BY_ROW));

    constant("HOGDescriptor_L2Hys", static_cast<long>(cv::HOGDescriptor::L2Hys));

    constant("HOUGH_GRADIENT", static_cast<long>(cv::HOUGH_GRADIENT));

    constant("HOUGH_GRADIENT_ALT", static_cast<long>(cv::HOUGH_GRADIENT_ALT));

    constant("HOUGH_MULTI_SCALE", static_cast<long>(cv::HOUGH_MULTI_SCALE));

    constant("HOUGH_PROBABILISTIC", static_cast<long>(cv::HOUGH_PROBABILISTIC));

    constant("HOUGH_STANDARD", static_cast<long>(cv::HOUGH_STANDARD));

    constant("INPAINT_NS", static_cast<long>(cv::INPAINT_NS));

    constant("INPAINT_TELEA", static_cast<long>(cv::INPAINT_TELEA));

    constant("INTERSECT_FULL", static_cast<long>(cv::INTERSECT_FULL));

    constant("INTERSECT_NONE", static_cast<long>(cv::INTERSECT_NONE));

    constant("INTERSECT_PARTIAL", static_cast<long>(cv::INTERSECT_PARTIAL));

    constant("INTER_AREA", static_cast<long>(cv::INTER_AREA));

    constant("INTER_BITS", static_cast<long>(cv::INTER_BITS));

    constant("INTER_BITS2", static_cast<long>(cv::INTER_BITS2));

    constant("INTER_CUBIC", static_cast<long>(cv::INTER_CUBIC));

    constant("INTER_LANCZOS4", static_cast<long>(cv::INTER_LANCZOS4));

    constant("INTER_LINEAR", static_cast<long>(cv::INTER_LINEAR));

    constant("INTER_LINEAR_EXACT", static_cast<long>(cv::INTER_LINEAR_EXACT));

    constant("INTER_MAX", static_cast<long>(cv::INTER_MAX));

    constant("INTER_NEAREST", static_cast<long>(cv::INTER_NEAREST));

    constant("INTER_NEAREST_EXACT", static_cast<long>(cv::INTER_NEAREST_EXACT));

    constant("INTER_TAB_SIZE", static_cast<long>(cv::INTER_TAB_SIZE));

    constant("INTER_TAB_SIZE2", static_cast<long>(cv::INTER_TAB_SIZE2));

    constant("KAZE_DIFF_CHARBONNIER", static_cast<long>(cv::KAZE::DIFF_CHARBONNIER));

    constant("KAZE_DIFF_PM_G1", static_cast<long>(cv::KAZE::DIFF_PM_G1));

    constant("KAZE_DIFF_PM_G2", static_cast<long>(cv::KAZE::DIFF_PM_G2));

    constant("KAZE_DIFF_WEICKERT", static_cast<long>(cv::KAZE::DIFF_WEICKERT));

    constant("KMEANS_PP_CENTERS", static_cast<long>(cv::KMEANS_PP_CENTERS));

    constant("KMEANS_RANDOM_CENTERS", static_cast<long>(cv::KMEANS_RANDOM_CENTERS));

    constant("KMEANS_USE_INITIAL_LABELS", static_cast<long>(cv::KMEANS_USE_INITIAL_LABELS));

    constant("LDR_SIZE", static_cast<long>(cv::LDR_SIZE));

    constant("LINE_4", static_cast<long>(cv::LINE_4));

    constant("LINE_8", static_cast<long>(cv::LINE_8));

    constant("LINE_AA", static_cast<long>(cv::LINE_AA));

    constant("LMEDS", static_cast<long>(cv::LMEDS));

    constant("LOCAL_OPTIM_GC", static_cast<long>(cv::LOCAL_OPTIM_GC));

    constant("LOCAL_OPTIM_INNER_AND_ITER_LO", static_cast<long>(cv::LOCAL_OPTIM_INNER_AND_ITER_LO));

    constant("LOCAL_OPTIM_INNER_LO", static_cast<long>(cv::LOCAL_OPTIM_INNER_LO));

    constant("LOCAL_OPTIM_NULL", static_cast<long>(cv::LOCAL_OPTIM_NULL));

    constant("LOCAL_OPTIM_SIGMA", static_cast<long>(cv::LOCAL_OPTIM_SIGMA));

    constant("LSD_REFINE_ADV", static_cast<long>(cv::LSD_REFINE_ADV));

    constant("LSD_REFINE_NONE", static_cast<long>(cv::LSD_REFINE_NONE));

    constant("LSD_REFINE_STD", static_cast<long>(cv::LSD_REFINE_STD));

    constant("LSQ_POLISHER", static_cast<long>(cv::LSQ_POLISHER));

    constant("MAGSAC", static_cast<long>(cv::MAGSAC));

    constant("MARKER_CROSS", static_cast<long>(cv::MARKER_CROSS));

    constant("MARKER_DIAMOND", static_cast<long>(cv::MARKER_DIAMOND));

    constant("MARKER_SQUARE", static_cast<long>(cv::MARKER_SQUARE));

    constant("MARKER_STAR", static_cast<long>(cv::MARKER_STAR));

    constant("MARKER_TILTED_CROSS", static_cast<long>(cv::MARKER_TILTED_CROSS));

    constant("MARKER_TRIANGLE_DOWN", static_cast<long>(cv::MARKER_TRIANGLE_DOWN));

    constant("MARKER_TRIANGLE_UP", static_cast<long>(cv::MARKER_TRIANGLE_UP));

    constant("MIXED_CLONE", static_cast<long>(cv::MIXED_CLONE));

    constant("MONOCHROME_TRANSFER", static_cast<long>(cv::MONOCHROME_TRANSFER));

    constant("MORPH_BLACKHAT", static_cast<long>(cv::MORPH_BLACKHAT));

    constant("MORPH_CLOSE", static_cast<long>(cv::MORPH_CLOSE));

    constant("MORPH_CROSS", static_cast<long>(cv::MORPH_CROSS));

    constant("MORPH_DILATE", static_cast<long>(cv::MORPH_DILATE));

    constant("MORPH_ELLIPSE", static_cast<long>(cv::MORPH_ELLIPSE));

    constant("MORPH_ERODE", static_cast<long>(cv::MORPH_ERODE));

    constant("MORPH_GRADIENT", static_cast<long>(cv::MORPH_GRADIENT));

    constant("MORPH_HITMISS", static_cast<long>(cv::MORPH_HITMISS));

    constant("MORPH_OPEN", static_cast<long>(cv::MORPH_OPEN));

    constant("MORPH_RECT", static_cast<long>(cv::MORPH_RECT));

    constant("MORPH_TOPHAT", static_cast<long>(cv::MORPH_TOPHAT));

    constant("MOTION_AFFINE", static_cast<long>(cv::MOTION_AFFINE));

    constant("MOTION_EUCLIDEAN", static_cast<long>(cv::MOTION_EUCLIDEAN));

    constant("MOTION_HOMOGRAPHY", static_cast<long>(cv::MOTION_HOMOGRAPHY));

    constant("MOTION_TRANSLATION", static_cast<long>(cv::MOTION_TRANSLATION));

    constant("Mat_AUTO_STEP", static_cast<long>(cv::Mat::AUTO_STEP));

    constant("Mat_CONTINUOUS_FLAG", static_cast<long>(cv::Mat::CONTINUOUS_FLAG));

    constant("Mat_DEPTH_MASK", static_cast<long>(cv::Mat::DEPTH_MASK));

    constant("Mat_MAGIC_MASK", static_cast<long>(cv::Mat::MAGIC_MASK));

    constant("Mat_MAGIC_VAL", static_cast<long>(cv::Mat::MAGIC_VAL));

    constant("Mat_SUBMATRIX_FLAG", static_cast<long>(cv::Mat::SUBMATRIX_FLAG));

    constant("Mat_TYPE_MASK", static_cast<long>(cv::Mat::TYPE_MASK));

    constant("NEIGH_FLANN_KNN", static_cast<long>(cv::NEIGH_FLANN_KNN));

    constant("NEIGH_FLANN_RADIUS", static_cast<long>(cv::NEIGH_FLANN_RADIUS));

    constant("NEIGH_GRID", static_cast<long>(cv::NEIGH_GRID));

    constant("NONE_POLISHER", static_cast<long>(cv::NONE_POLISHER));

    constant("NORMAL_CLONE", static_cast<long>(cv::NORMAL_CLONE));

    constant("NORMCONV_FILTER", static_cast<long>(cv::NORMCONV_FILTER));

    constant("NORM_HAMMING", static_cast<long>(cv::NORM_HAMMING));

    constant("NORM_HAMMING2", static_cast<long>(cv::NORM_HAMMING2));

    constant("NORM_INF", static_cast<long>(cv::NORM_INF));

    constant("NORM_L1", static_cast<long>(cv::NORM_L1));

    constant("NORM_L2", static_cast<long>(cv::NORM_L2));

    constant("NORM_L2SQR", static_cast<long>(cv::NORM_L2SQR));

    constant("NORM_MINMAX", static_cast<long>(cv::NORM_MINMAX));

    constant("NORM_RELATIVE", static_cast<long>(cv::NORM_RELATIVE));

    constant("NORM_TYPE_MASK", static_cast<long>(cv::NORM_TYPE_MASK));

    constant("OPTFLOW_FARNEBACK_GAUSSIAN", static_cast<long>(cv::OPTFLOW_FARNEBACK_GAUSSIAN));

    constant("OPTFLOW_LK_GET_MIN_EIGENVALS", static_cast<long>(cv::OPTFLOW_LK_GET_MIN_EIGENVALS));

    constant("OPTFLOW_USE_INITIAL_FLOW", static_cast<long>(cv::OPTFLOW_USE_INITIAL_FLOW));

    constant("ORB_FAST_SCORE", static_cast<long>(cv::ORB::FAST_SCORE));

    constant("ORB_HARRIS_SCORE", static_cast<long>(cv::ORB::HARRIS_SCORE));

    constant("PCA_DATA_AS_COL", static_cast<long>(cv::PCA::DATA_AS_COL));

    constant("PCA_DATA_AS_ROW", static_cast<long>(cv::PCA::DATA_AS_ROW));

    constant("PCA_USE_AVG", static_cast<long>(cv::PCA::USE_AVG));

    constant("PROJ_SPHERICAL_EQRECT", static_cast<long>(cv::PROJ_SPHERICAL_EQRECT));

    constant("PROJ_SPHERICAL_ORTHO", static_cast<long>(cv::PROJ_SPHERICAL_ORTHO));

    constant("Param_ALGORITHM", static_cast<long>(cv::Param::ALGORITHM));

    constant("Param_BOOLEAN", static_cast<long>(cv::Param::BOOLEAN));

    constant("Param_FLOAT", static_cast<long>(cv::Param::FLOAT));

    constant("Param_INT", static_cast<long>(cv::Param::INT));

    constant("Param_MAT", static_cast<long>(cv::Param::MAT));

    constant("Param_MAT_VECTOR", static_cast<long>(cv::Param::MAT_VECTOR));

    constant("Param_REAL", static_cast<long>(cv::Param::REAL));

    constant("Param_SCALAR", static_cast<long>(cv::Param::SCALAR));

    constant("Param_STRING", static_cast<long>(cv::Param::STRING));

    constant("Param_UCHAR", static_cast<long>(cv::Param::UCHAR));

    constant("Param_UINT64", static_cast<long>(cv::Param::UINT64));

    constant("Param_UNSIGNED_INT", static_cast<long>(cv::Param::UNSIGNED_INT));

    constant("QRCodeEncoder_CORRECT_LEVEL_H", static_cast<long>(cv::QRCodeEncoder::CORRECT_LEVEL_H));

    constant("QRCodeEncoder_CORRECT_LEVEL_L", static_cast<long>(cv::QRCodeEncoder::CORRECT_LEVEL_L));

    constant("QRCodeEncoder_CORRECT_LEVEL_M", static_cast<long>(cv::QRCodeEncoder::CORRECT_LEVEL_M));

    constant("QRCodeEncoder_CORRECT_LEVEL_Q", static_cast<long>(cv::QRCodeEncoder::CORRECT_LEVEL_Q));

    constant("QRCodeEncoder_ECI_UTF8", static_cast<long>(cv::QRCodeEncoder::ECI_UTF8));

    constant("QRCodeEncoder_MODE_ALPHANUMERIC", static_cast<long>(cv::QRCodeEncoder::MODE_ALPHANUMERIC));

    constant("QRCodeEncoder_MODE_AUTO", static_cast<long>(cv::QRCodeEncoder::MODE_AUTO));

    constant("QRCodeEncoder_MODE_BYTE", static_cast<long>(cv::QRCodeEncoder::MODE_BYTE));

    constant("QRCodeEncoder_MODE_ECI", static_cast<long>(cv::QRCodeEncoder::MODE_ECI));

    constant("QRCodeEncoder_MODE_KANJI", static_cast<long>(cv::QRCodeEncoder::MODE_KANJI));

    constant("QRCodeEncoder_MODE_NUMERIC", static_cast<long>(cv::QRCodeEncoder::MODE_NUMERIC));

    constant("QRCodeEncoder_MODE_STRUCTURED_APPEND", static_cast<long>(cv::QRCodeEncoder::MODE_STRUCTURED_APPEND));

    constant("QUAT_ASSUME_NOT_UNIT", static_cast<long>(cv::QUAT_ASSUME_NOT_UNIT));

    constant("QUAT_ASSUME_UNIT", static_cast<long>(cv::QUAT_ASSUME_UNIT));

    constant("QuatEnum_EULER_ANGLES_MAX_VALUE", static_cast<long>(cv::QuatEnum::EULER_ANGLES_MAX_VALUE));

    constant("QuatEnum_EXT_XYX", static_cast<long>(cv::QuatEnum::EXT_XYX));

    constant("QuatEnum_EXT_XYZ", static_cast<long>(cv::QuatEnum::EXT_XYZ));

    constant("QuatEnum_EXT_XZX", static_cast<long>(cv::QuatEnum::EXT_XZX));

    constant("QuatEnum_EXT_XZY", static_cast<long>(cv::QuatEnum::EXT_XZY));

    constant("QuatEnum_EXT_YXY", static_cast<long>(cv::QuatEnum::EXT_YXY));

    constant("QuatEnum_EXT_YXZ", static_cast<long>(cv::QuatEnum::EXT_YXZ));

    constant("QuatEnum_EXT_YZX", static_cast<long>(cv::QuatEnum::EXT_YZX));

    constant("QuatEnum_EXT_YZY", static_cast<long>(cv::QuatEnum::EXT_YZY));

    constant("QuatEnum_EXT_ZXY", static_cast<long>(cv::QuatEnum::EXT_ZXY));

    constant("QuatEnum_EXT_ZXZ", static_cast<long>(cv::QuatEnum::EXT_ZXZ));

    constant("QuatEnum_EXT_ZYX", static_cast<long>(cv::QuatEnum::EXT_ZYX));

    constant("QuatEnum_EXT_ZYZ", static_cast<long>(cv::QuatEnum::EXT_ZYZ));

    constant("QuatEnum_INT_XYX", static_cast<long>(cv::QuatEnum::INT_XYX));

    constant("QuatEnum_INT_XYZ", static_cast<long>(cv::QuatEnum::INT_XYZ));

    constant("QuatEnum_INT_XZX", static_cast<long>(cv::QuatEnum::INT_XZX));

    constant("QuatEnum_INT_XZY", static_cast<long>(cv::QuatEnum::INT_XZY));

    constant("QuatEnum_INT_YXY", static_cast<long>(cv::QuatEnum::INT_YXY));

    constant("QuatEnum_INT_YXZ", static_cast<long>(cv::QuatEnum::INT_YXZ));

    constant("QuatEnum_INT_YZX", static_cast<long>(cv::QuatEnum::INT_YZX));

    constant("QuatEnum_INT_YZY", static_cast<long>(cv::QuatEnum::INT_YZY));

    constant("QuatEnum_INT_ZXY", static_cast<long>(cv::QuatEnum::INT_ZXY));

    constant("QuatEnum_INT_ZXZ", static_cast<long>(cv::QuatEnum::INT_ZXZ));

    constant("QuatEnum_INT_ZYX", static_cast<long>(cv::QuatEnum::INT_ZYX));

    constant("QuatEnum_INT_ZYZ", static_cast<long>(cv::QuatEnum::INT_ZYZ));

    constant("RANSAC", static_cast<long>(cv::RANSAC));

    constant("RECURS_FILTER", static_cast<long>(cv::RECURS_FILTER));

    constant("REDUCE_AVG", static_cast<long>(cv::REDUCE_AVG));

    constant("REDUCE_MAX", static_cast<long>(cv::REDUCE_MAX));

    constant("REDUCE_MIN", static_cast<long>(cv::REDUCE_MIN));

    constant("REDUCE_SUM", static_cast<long>(cv::REDUCE_SUM));

    constant("REDUCE_SUM2", static_cast<long>(cv::REDUCE_SUM2));

    constant("RETR_CCOMP", static_cast<long>(cv::RETR_CCOMP));

    constant("RETR_EXTERNAL", static_cast<long>(cv::RETR_EXTERNAL));

    constant("RETR_FLOODFILL", static_cast<long>(cv::RETR_FLOODFILL));

    constant("RETR_LIST", static_cast<long>(cv::RETR_LIST));

    constant("RETR_TREE", static_cast<long>(cv::RETR_TREE));

    constant("RHO", static_cast<long>(cv::RHO));

    constant("RNG_NORMAL", static_cast<long>(cv::RNG::NORMAL));

    constant("RNG_UNIFORM", static_cast<long>(cv::RNG::UNIFORM));

    constant("ROTATE_180", static_cast<long>(cv::ROTATE_180));

    constant("ROTATE_90_CLOCKWISE", static_cast<long>(cv::ROTATE_90_CLOCKWISE));

    constant("ROTATE_90_COUNTERCLOCKWISE", static_cast<long>(cv::ROTATE_90_COUNTERCLOCKWISE));

    constant("SAMPLING_NAPSAC", static_cast<long>(cv::SAMPLING_NAPSAC));

    constant("SAMPLING_PROGRESSIVE_NAPSAC", static_cast<long>(cv::SAMPLING_PROGRESSIVE_NAPSAC));

    constant("SAMPLING_PROSAC", static_cast<long>(cv::SAMPLING_PROSAC));

    constant("SAMPLING_UNIFORM", static_cast<long>(cv::SAMPLING_UNIFORM));

    constant("SCORE_METHOD_LMEDS", static_cast<long>(cv::SCORE_METHOD_LMEDS));

    constant("SCORE_METHOD_MAGSAC", static_cast<long>(cv::SCORE_METHOD_MAGSAC));

    constant("SCORE_METHOD_MSAC", static_cast<long>(cv::SCORE_METHOD_MSAC));

    constant("SCORE_METHOD_RANSAC", static_cast<long>(cv::SCORE_METHOD_RANSAC));

    constant("SOLVELP_LOST", static_cast<long>(cv::SOLVELP_LOST));

    constant("SOLVELP_MULTI", static_cast<long>(cv::SOLVELP_MULTI));

    constant("SOLVELP_SINGLE", static_cast<long>(cv::SOLVELP_SINGLE));

    constant("SOLVELP_UNBOUNDED", static_cast<long>(cv::SOLVELP_UNBOUNDED));

    constant("SOLVELP_UNFEASIBLE", static_cast<long>(cv::SOLVELP_UNFEASIBLE));

    constant("SOLVEPNP_AP3P", static_cast<long>(cv::SOLVEPNP_AP3P));

    constant("SOLVEPNP_DLS", static_cast<long>(cv::SOLVEPNP_DLS));

    constant("SOLVEPNP_EPNP", static_cast<long>(cv::SOLVEPNP_EPNP));

    constant("SOLVEPNP_IPPE", static_cast<long>(cv::SOLVEPNP_IPPE));

    constant("SOLVEPNP_IPPE_SQUARE", static_cast<long>(cv::SOLVEPNP_IPPE_SQUARE));

    constant("SOLVEPNP_ITERATIVE", static_cast<long>(cv::SOLVEPNP_ITERATIVE));

    constant("SOLVEPNP_MAX_COUNT", static_cast<long>(cv::SOLVEPNP_MAX_COUNT));

    constant("SOLVEPNP_P3P", static_cast<long>(cv::SOLVEPNP_P3P));

    constant("SOLVEPNP_SQPNP", static_cast<long>(cv::SOLVEPNP_SQPNP));

    constant("SOLVEPNP_UPNP", static_cast<long>(cv::SOLVEPNP_UPNP));

    constant("SORT_ASCENDING", static_cast<long>(cv::SORT_ASCENDING));

    constant("SORT_DESCENDING", static_cast<long>(cv::SORT_DESCENDING));

    constant("SORT_EVERY_COLUMN", static_cast<long>(cv::SORT_EVERY_COLUMN));

    constant("SORT_EVERY_ROW", static_cast<long>(cv::SORT_EVERY_ROW));

    constant("SVD_FULL_UV", static_cast<long>(cv::SVD::FULL_UV));

    constant("SVD_MODIFY_A", static_cast<long>(cv::SVD::MODIFY_A));

    constant("SVD_NO_UV", static_cast<long>(cv::SVD::NO_UV));

    constant("SparseMat_HASH_BIT", static_cast<long>(cv::SparseMat::HASH_BIT));

    constant("SparseMat_HASH_SCALE", static_cast<long>(cv::SparseMat::HASH_SCALE));

    constant("SparseMat_MAGIC_VAL", static_cast<long>(cv::SparseMat::MAGIC_VAL));

    constant("SparseMat_MAX_DIM", static_cast<long>(cv::SparseMat::MAX_DIM));

    constant("StereoBM_PREFILTER_NORMALIZED_RESPONSE", static_cast<long>(cv::StereoBM::PREFILTER_NORMALIZED_RESPONSE));

    constant("StereoBM_PREFILTER_XSOBEL", static_cast<long>(cv::StereoBM::PREFILTER_XSOBEL));

    constant("StereoMatcher_DISP_SCALE", static_cast<long>(cv::StereoMatcher::DISP_SCALE));

    constant("StereoMatcher_DISP_SHIFT", static_cast<long>(cv::StereoMatcher::DISP_SHIFT));

    constant("StereoSGBM_MODE_HH", static_cast<long>(cv::StereoSGBM::MODE_HH));

    constant("StereoSGBM_MODE_HH4", static_cast<long>(cv::StereoSGBM::MODE_HH4));

    constant("StereoSGBM_MODE_SGBM", static_cast<long>(cv::StereoSGBM::MODE_SGBM));

    constant("StereoSGBM_MODE_SGBM_3WAY", static_cast<long>(cv::StereoSGBM::MODE_SGBM_3WAY));

    constant("Subdiv2D_NEXT_AROUND_DST", static_cast<long>(cv::Subdiv2D::NEXT_AROUND_DST));

    constant("Subdiv2D_NEXT_AROUND_LEFT", static_cast<long>(cv::Subdiv2D::NEXT_AROUND_LEFT));

    constant("Subdiv2D_NEXT_AROUND_ORG", static_cast<long>(cv::Subdiv2D::NEXT_AROUND_ORG));

    constant("Subdiv2D_NEXT_AROUND_RIGHT", static_cast<long>(cv::Subdiv2D::NEXT_AROUND_RIGHT));

    constant("Subdiv2D_PREV_AROUND_DST", static_cast<long>(cv::Subdiv2D::PREV_AROUND_DST));

    constant("Subdiv2D_PREV_AROUND_LEFT", static_cast<long>(cv::Subdiv2D::PREV_AROUND_LEFT));

    constant("Subdiv2D_PREV_AROUND_ORG", static_cast<long>(cv::Subdiv2D::PREV_AROUND_ORG));

    constant("Subdiv2D_PREV_AROUND_RIGHT", static_cast<long>(cv::Subdiv2D::PREV_AROUND_RIGHT));

    constant("Subdiv2D_PTLOC_ERROR", static_cast<long>(cv::Subdiv2D::PTLOC_ERROR));

    constant("Subdiv2D_PTLOC_INSIDE", static_cast<long>(cv::Subdiv2D::PTLOC_INSIDE));

    constant("Subdiv2D_PTLOC_ON_EDGE", static_cast<long>(cv::Subdiv2D::PTLOC_ON_EDGE));

    constant("Subdiv2D_PTLOC_OUTSIDE_RECT", static_cast<long>(cv::Subdiv2D::PTLOC_OUTSIDE_RECT));

    constant("Subdiv2D_PTLOC_VERTEX", static_cast<long>(cv::Subdiv2D::PTLOC_VERTEX));

    constant("THRESH_BINARY", static_cast<long>(cv::THRESH_BINARY));

    constant("THRESH_BINARY_INV", static_cast<long>(cv::THRESH_BINARY_INV));

    constant("THRESH_MASK", static_cast<long>(cv::THRESH_MASK));

    constant("THRESH_OTSU", static_cast<long>(cv::THRESH_OTSU));

    constant("THRESH_TOZERO", static_cast<long>(cv::THRESH_TOZERO));

    constant("THRESH_TOZERO_INV", static_cast<long>(cv::THRESH_TOZERO_INV));

    constant("THRESH_TRIANGLE", static_cast<long>(cv::THRESH_TRIANGLE));

    constant("THRESH_TRUNC", static_cast<long>(cv::THRESH_TRUNC));

    constant("TM_CCOEFF", static_cast<long>(cv::TM_CCOEFF));

    constant("TM_CCOEFF_NORMED", static_cast<long>(cv::TM_CCOEFF_NORMED));

    constant("TM_CCORR", static_cast<long>(cv::TM_CCORR));

    constant("TM_CCORR_NORMED", static_cast<long>(cv::TM_CCORR_NORMED));

    constant("TM_SQDIFF", static_cast<long>(cv::TM_SQDIFF));

    constant("TM_SQDIFF_NORMED", static_cast<long>(cv::TM_SQDIFF_NORMED));

    constant("TermCriteria_COUNT", static_cast<long>(cv::TermCriteria::COUNT));

    constant("TermCriteria_EPS", static_cast<long>(cv::TermCriteria::EPS));

    constant("TermCriteria_MAX_ITER", static_cast<long>(cv::TermCriteria::MAX_ITER));

    constant("UMatData_ASYNC_CLEANUP", static_cast<long>(cv::UMatData::ASYNC_CLEANUP));

    constant("UMatData_COPY_ON_MAP", static_cast<long>(cv::UMatData::COPY_ON_MAP));

    constant("UMatData_DEVICE_COPY_OBSOLETE", static_cast<long>(cv::UMatData::DEVICE_COPY_OBSOLETE));

    constant("UMatData_DEVICE_MEM_MAPPED", static_cast<long>(cv::UMatData::DEVICE_MEM_MAPPED));

    constant("UMatData_HOST_COPY_OBSOLETE", static_cast<long>(cv::UMatData::HOST_COPY_OBSOLETE));

    constant("UMatData_TEMP_COPIED_UMAT", static_cast<long>(cv::UMatData::TEMP_COPIED_UMAT));

    constant("UMatData_TEMP_UMAT", static_cast<long>(cv::UMatData::TEMP_UMAT));

    constant("UMatData_USER_ALLOCATED", static_cast<long>(cv::UMatData::USER_ALLOCATED));

    constant("UMat_AUTO_STEP", static_cast<long>(cv::UMat::AUTO_STEP));

    constant("UMat_CONTINUOUS_FLAG", static_cast<long>(cv::UMat::CONTINUOUS_FLAG));

    constant("UMat_DEPTH_MASK", static_cast<long>(cv::UMat::DEPTH_MASK));

    constant("UMat_MAGIC_MASK", static_cast<long>(cv::UMat::MAGIC_MASK));

    constant("UMat_MAGIC_VAL", static_cast<long>(cv::UMat::MAGIC_VAL));

    constant("UMat_SUBMATRIX_FLAG", static_cast<long>(cv::UMat::SUBMATRIX_FLAG));

    constant("UMat_TYPE_MASK", static_cast<long>(cv::UMat::TYPE_MASK));

    constant("USAC_ACCURATE", static_cast<long>(cv::USAC_ACCURATE));

    constant("USAC_DEFAULT", static_cast<long>(cv::USAC_DEFAULT));

    constant("USAC_FAST", static_cast<long>(cv::USAC_FAST));

    constant("USAC_FM_8PTS", static_cast<long>(cv::USAC_FM_8PTS));

    constant("USAC_MAGSAC", static_cast<long>(cv::USAC_MAGSAC));

    constant("USAC_PARALLEL", static_cast<long>(cv::USAC_PARALLEL));

    constant("USAC_PROSAC", static_cast<long>(cv::USAC_PROSAC));

    constant("USAGE_ALLOCATE_DEVICE_MEMORY", static_cast<long>(cv::USAGE_ALLOCATE_DEVICE_MEMORY));

    constant("USAGE_ALLOCATE_HOST_MEMORY", static_cast<long>(cv::USAGE_ALLOCATE_HOST_MEMORY));

    constant("USAGE_ALLOCATE_SHARED_MEMORY", static_cast<long>(cv::USAGE_ALLOCATE_SHARED_MEMORY));

    constant("USAGE_DEFAULT", static_cast<long>(cv::USAGE_DEFAULT));

    constant("WARP_FILL_OUTLIERS", static_cast<long>(cv::WARP_FILL_OUTLIERS));

    constant("WARP_INVERSE_MAP", static_cast<long>(cv::WARP_INVERSE_MAP));

    constant("WARP_POLAR_LINEAR", static_cast<long>(cv::WARP_POLAR_LINEAR));

    constant("WARP_POLAR_LOG", static_cast<long>(cv::WARP_POLAR_LOG));

    constant("WARP_RELATIVE_MAP", static_cast<long>(cv::WARP_RELATIVE_MAP));

    constant("_InputArray_CUDA_GPU_MAT", static_cast<long>(cv::_InputArray::CUDA_GPU_MAT));

    constant("_InputArray_CUDA_HOST_MEM", static_cast<long>(cv::_InputArray::CUDA_HOST_MEM));

    constant("_InputArray_EXPR", static_cast<long>(cv::_InputArray::EXPR));

    constant("_InputArray_FIXED_SIZE", static_cast<long>(cv::_InputArray::FIXED_SIZE));

    constant("_InputArray_FIXED_TYPE", static_cast<long>(cv::_InputArray::FIXED_TYPE));

    constant("_InputArray_KIND_MASK", static_cast<long>(cv::_InputArray::KIND_MASK));

    constant("_InputArray_KIND_SHIFT", static_cast<long>(cv::_InputArray::KIND_SHIFT));

    constant("_InputArray_MAT", static_cast<long>(cv::_InputArray::MAT));

    constant("_InputArray_MATX", static_cast<long>(cv::_InputArray::MATX));

    constant("_InputArray_NONE", static_cast<long>(cv::_InputArray::NONE));

    constant("_InputArray_OPENGL_BUFFER", static_cast<long>(cv::_InputArray::OPENGL_BUFFER));

    constant("_InputArray_STD_ARRAY", static_cast<long>(cv::_InputArray::STD_ARRAY));

    constant("_InputArray_STD_ARRAY_MAT", static_cast<long>(cv::_InputArray::STD_ARRAY_MAT));

    constant("_InputArray_STD_BOOL_VECTOR", static_cast<long>(cv::_InputArray::STD_BOOL_VECTOR));

    constant("_InputArray_STD_VECTOR", static_cast<long>(cv::_InputArray::STD_VECTOR));

    constant("_InputArray_STD_VECTOR_CUDA_GPU_MAT", static_cast<long>(cv::_InputArray::STD_VECTOR_CUDA_GPU_MAT));

    constant("_InputArray_STD_VECTOR_MAT", static_cast<long>(cv::_InputArray::STD_VECTOR_MAT));

    constant("_InputArray_STD_VECTOR_UMAT", static_cast<long>(cv::_InputArray::STD_VECTOR_UMAT));

    constant("_InputArray_STD_VECTOR_VECTOR", static_cast<long>(cv::_InputArray::STD_VECTOR_VECTOR));

    constant("_InputArray_UMAT", static_cast<long>(cv::_InputArray::UMAT));

    constant("_OutputArray_DEPTH_MASK_16F", static_cast<long>(cv::_OutputArray::DEPTH_MASK_16F));

    constant("_OutputArray_DEPTH_MASK_16S", static_cast<long>(cv::_OutputArray::DEPTH_MASK_16S));

    constant("_OutputArray_DEPTH_MASK_16U", static_cast<long>(cv::_OutputArray::DEPTH_MASK_16U));

    constant("_OutputArray_DEPTH_MASK_32F", static_cast<long>(cv::_OutputArray::DEPTH_MASK_32F));

    constant("_OutputArray_DEPTH_MASK_32S", static_cast<long>(cv::_OutputArray::DEPTH_MASK_32S));

    constant("_OutputArray_DEPTH_MASK_64F", static_cast<long>(cv::_OutputArray::DEPTH_MASK_64F));

    constant("_OutputArray_DEPTH_MASK_8S", static_cast<long>(cv::_OutputArray::DEPTH_MASK_8S));

    constant("_OutputArray_DEPTH_MASK_8U", static_cast<long>(cv::_OutputArray::DEPTH_MASK_8U));

    constant("_OutputArray_DEPTH_MASK_ALL", static_cast<long>(cv::_OutputArray::DEPTH_MASK_ALL));

    constant("_OutputArray_DEPTH_MASK_ALL_16F", static_cast<long>(cv::_OutputArray::DEPTH_MASK_ALL_16F));

    constant("_OutputArray_DEPTH_MASK_ALL_BUT_8S", static_cast<long>(cv::_OutputArray::DEPTH_MASK_ALL_BUT_8S));

    constant("_OutputArray_DEPTH_MASK_FLT", static_cast<long>(cv::_OutputArray::DEPTH_MASK_FLT));

    constant("__UMAT_USAGE_FLAGS_32BIT", static_cast<long>(cv::__UMAT_USAGE_FLAGS_32BIT));

    constant("BadAlign", static_cast<long>(cv::Error::BadAlign));

    constant("BadAlphaChannel", static_cast<long>(cv::Error::BadAlphaChannel));

    constant("BadCOI", static_cast<long>(cv::Error::BadCOI));

    constant("BadCallBack", static_cast<long>(cv::Error::BadCallBack));

    constant("BadDataPtr", static_cast<long>(cv::Error::BadDataPtr));

    constant("BadDepth", static_cast<long>(cv::Error::BadDepth));

    constant("BadImageSize", static_cast<long>(cv::Error::BadImageSize));

    constant("BadModelOrChSeq", static_cast<long>(cv::Error::BadModelOrChSeq));

    constant("BadNumChannel1U", static_cast<long>(cv::Error::BadNumChannel1U));

    constant("BadNumChannels", static_cast<long>(cv::Error::BadNumChannels));

    constant("BadOffset", static_cast<long>(cv::Error::BadOffset));

    constant("BadOrder", static_cast<long>(cv::Error::BadOrder));

    constant("BadOrigin", static_cast<long>(cv::Error::BadOrigin));

    constant("BadROISize", static_cast<long>(cv::Error::BadROISize));

    constant("BadStep", static_cast<long>(cv::Error::BadStep));

    constant("BadTileSize", static_cast<long>(cv::Error::BadTileSize));

    constant("GpuApiCallError", static_cast<long>(cv::Error::GpuApiCallError));

    constant("GpuNotSupported", static_cast<long>(cv::Error::GpuNotSupported));

    constant("HeaderIsNull", static_cast<long>(cv::Error::HeaderIsNull));

    constant("MaskIsTiled", static_cast<long>(cv::Error::MaskIsTiled));

    constant("OpenCLApiCallError", static_cast<long>(cv::Error::OpenCLApiCallError));

    constant("OpenCLDoubleNotSupported", static_cast<long>(cv::Error::OpenCLDoubleNotSupported));

    constant("OpenCLInitError", static_cast<long>(cv::Error::OpenCLInitError));

    constant("OpenCLNoAMDBlasFft", static_cast<long>(cv::Error::OpenCLNoAMDBlasFft));

    constant("OpenGlApiCallError", static_cast<long>(cv::Error::OpenGlApiCallError));

    constant("OpenGlNotSupported", static_cast<long>(cv::Error::OpenGlNotSupported));

    constant("StsAssert", static_cast<long>(cv::Error::StsAssert));

    constant("StsAutoTrace", static_cast<long>(cv::Error::StsAutoTrace));

    constant("StsBackTrace", static_cast<long>(cv::Error::StsBackTrace));

    constant("StsBadArg", static_cast<long>(cv::Error::StsBadArg));

    constant("StsBadFlag", static_cast<long>(cv::Error::StsBadFlag));

    constant("StsBadFunc", static_cast<long>(cv::Error::StsBadFunc));

    constant("StsBadMask", static_cast<long>(cv::Error::StsBadMask));

    constant("StsBadMemBlock", static_cast<long>(cv::Error::StsBadMemBlock));

    constant("StsBadPoint", static_cast<long>(cv::Error::StsBadPoint));

    constant("StsBadSize", static_cast<long>(cv::Error::StsBadSize));

    constant("StsDivByZero", static_cast<long>(cv::Error::StsDivByZero));

    constant("StsError", static_cast<long>(cv::Error::StsError));

    constant("StsFilterOffsetErr", static_cast<long>(cv::Error::StsFilterOffsetErr));

    constant("StsFilterStructContentErr", static_cast<long>(cv::Error::StsFilterStructContentErr));

    constant("StsInplaceNotSupported", static_cast<long>(cv::Error::StsInplaceNotSupported));

    constant("StsInternal", static_cast<long>(cv::Error::StsInternal));

    constant("StsKernelStructContentErr", static_cast<long>(cv::Error::StsKernelStructContentErr));

    constant("StsNoConv", static_cast<long>(cv::Error::StsNoConv));

    constant("StsNoMem", static_cast<long>(cv::Error::StsNoMem));

    constant("StsNotImplemented", static_cast<long>(cv::Error::StsNotImplemented));

    constant("StsNullPtr", static_cast<long>(cv::Error::StsNullPtr));

    constant("StsObjectNotFound", static_cast<long>(cv::Error::StsObjectNotFound));

    constant("StsOk", static_cast<long>(cv::Error::StsOk));

    constant("StsOutOfRange", static_cast<long>(cv::Error::StsOutOfRange));

    constant("StsParseError", static_cast<long>(cv::Error::StsParseError));

    constant("StsUnmatchedFormats", static_cast<long>(cv::Error::StsUnmatchedFormats));

    constant("StsUnmatchedSizes", static_cast<long>(cv::Error::StsUnmatchedSizes));

    constant("StsUnsupportedFormat", static_cast<long>(cv::Error::StsUnsupportedFormat));

    constant("StsVecLengthErr", static_cast<long>(cv::Error::StsVecLengthErr));

    constant("CORNER_REFINE_APRILTAG", static_cast<long>(cv::aruco::CORNER_REFINE_APRILTAG));

    constant("CORNER_REFINE_CONTOUR", static_cast<long>(cv::aruco::CORNER_REFINE_CONTOUR));

    constant("CORNER_REFINE_NONE", static_cast<long>(cv::aruco::CORNER_REFINE_NONE));

    constant("CORNER_REFINE_SUBPIX", static_cast<long>(cv::aruco::CORNER_REFINE_SUBPIX));

    constant("DICT_4X4_100", static_cast<long>(cv::aruco::DICT_4X4_100));

    constant("DICT_4X4_1000", static_cast<long>(cv::aruco::DICT_4X4_1000));

    constant("DICT_4X4_250", static_cast<long>(cv::aruco::DICT_4X4_250));

    constant("DICT_4X4_50", static_cast<long>(cv::aruco::DICT_4X4_50));

    constant("DICT_5X5_100", static_cast<long>(cv::aruco::DICT_5X5_100));

    constant("DICT_5X5_1000", static_cast<long>(cv::aruco::DICT_5X5_1000));

    constant("DICT_5X5_250", static_cast<long>(cv::aruco::DICT_5X5_250));

    constant("DICT_5X5_50", static_cast<long>(cv::aruco::DICT_5X5_50));

    constant("DICT_6X6_100", static_cast<long>(cv::aruco::DICT_6X6_100));

    constant("DICT_6X6_1000", static_cast<long>(cv::aruco::DICT_6X6_1000));

    constant("DICT_6X6_250", static_cast<long>(cv::aruco::DICT_6X6_250));

    constant("DICT_6X6_50", static_cast<long>(cv::aruco::DICT_6X6_50));

    constant("DICT_7X7_100", static_cast<long>(cv::aruco::DICT_7X7_100));

    constant("DICT_7X7_1000", static_cast<long>(cv::aruco::DICT_7X7_1000));

    constant("DICT_7X7_250", static_cast<long>(cv::aruco::DICT_7X7_250));

    constant("DICT_7X7_50", static_cast<long>(cv::aruco::DICT_7X7_50));

    constant("DICT_APRILTAG_16h5", static_cast<long>(cv::aruco::DICT_APRILTAG_16h5));

    constant("DICT_APRILTAG_25h9", static_cast<long>(cv::aruco::DICT_APRILTAG_25h9));

    constant("DICT_APRILTAG_36h10", static_cast<long>(cv::aruco::DICT_APRILTAG_36h10));

    constant("DICT_APRILTAG_36h11", static_cast<long>(cv::aruco::DICT_APRILTAG_36h11));

    constant("DICT_ARUCO_MIP_36h12", static_cast<long>(cv::aruco::DICT_ARUCO_MIP_36h12));

    constant("DICT_ARUCO_ORIGINAL", static_cast<long>(cv::aruco::DICT_ARUCO_ORIGINAL));

    constant("TEST_CUSTOM", static_cast<long>(cv::detail::TEST_CUSTOM));

    constant("TEST_EQ", static_cast<long>(cv::detail::TEST_EQ));

    constant("TEST_GE", static_cast<long>(cv::detail::TEST_GE));

    constant("TEST_GT", static_cast<long>(cv::detail::TEST_GT));

    constant("TEST_LE", static_cast<long>(cv::detail::TEST_LE));

    constant("TEST_LT", static_cast<long>(cv::detail::TEST_LT));

    constant("TEST_NE", static_cast<long>(cv::detail::TEST_NE));

    constant("TrackerSamplerCSC_MODE_DETECT", static_cast<long>(cv::detail::TrackerSamplerCSC::MODE_DETECT));

    constant("TrackerSamplerCSC_MODE_INIT_NEG", static_cast<long>(cv::detail::TrackerSamplerCSC::MODE_INIT_NEG));

    constant("TrackerSamplerCSC_MODE_INIT_POS", static_cast<long>(cv::detail::TrackerSamplerCSC::MODE_INIT_POS));

    constant("TrackerSamplerCSC_MODE_TRACK_NEG", static_cast<long>(cv::detail::TrackerSamplerCSC::MODE_TRACK_NEG));

    constant("TrackerSamplerCSC_MODE_TRACK_POS", static_cast<long>(cv::detail::TrackerSamplerCSC::MODE_TRACK_POS));

    constant("DNN_BACKEND_CANN", static_cast<long>(cv::dnn::DNN_BACKEND_CANN));

    constant("DNN_BACKEND_CUDA", static_cast<long>(cv::dnn::DNN_BACKEND_CUDA));

    constant("DNN_BACKEND_DEFAULT", static_cast<long>(cv::dnn::DNN_BACKEND_DEFAULT));

    constant("DNN_BACKEND_HALIDE", static_cast<long>(cv::dnn::DNN_BACKEND_HALIDE));

    constant("DNN_BACKEND_INFERENCE_ENGINE", static_cast<long>(cv::dnn::DNN_BACKEND_INFERENCE_ENGINE));

    constant("DNN_BACKEND_OPENCV", static_cast<long>(cv::dnn::DNN_BACKEND_OPENCV));

    constant("DNN_BACKEND_TIMVX", static_cast<long>(cv::dnn::DNN_BACKEND_TIMVX));

    constant("DNN_BACKEND_VKCOM", static_cast<long>(cv::dnn::DNN_BACKEND_VKCOM));

    constant("DNN_BACKEND_WEBNN", static_cast<long>(cv::dnn::DNN_BACKEND_WEBNN));

    constant("DNN_LAYOUT_NCDHW", static_cast<long>(cv::dnn::DNN_LAYOUT_NCDHW));

    constant("DNN_LAYOUT_NCHW", static_cast<long>(cv::dnn::DNN_LAYOUT_NCHW));

    constant("DNN_LAYOUT_ND", static_cast<long>(cv::dnn::DNN_LAYOUT_ND));

    constant("DNN_LAYOUT_NDHWC", static_cast<long>(cv::dnn::DNN_LAYOUT_NDHWC));

    constant("DNN_LAYOUT_NHWC", static_cast<long>(cv::dnn::DNN_LAYOUT_NHWC));

    constant("DNN_LAYOUT_PLANAR", static_cast<long>(cv::dnn::DNN_LAYOUT_PLANAR));

    constant("DNN_LAYOUT_UNKNOWN", static_cast<long>(cv::dnn::DNN_LAYOUT_UNKNOWN));

    constant("DNN_PMODE_CROP_CENTER", static_cast<long>(cv::dnn::DNN_PMODE_CROP_CENTER));

    constant("DNN_PMODE_LETTERBOX", static_cast<long>(cv::dnn::DNN_PMODE_LETTERBOX));

    constant("DNN_PMODE_NULL", static_cast<long>(cv::dnn::DNN_PMODE_NULL));

    constant("DNN_TARGET_CPU", static_cast<long>(cv::dnn::DNN_TARGET_CPU));

    constant("DNN_TARGET_CPU_FP16", static_cast<long>(cv::dnn::DNN_TARGET_CPU_FP16));

    constant("DNN_TARGET_CUDA", static_cast<long>(cv::dnn::DNN_TARGET_CUDA));

    constant("DNN_TARGET_CUDA_FP16", static_cast<long>(cv::dnn::DNN_TARGET_CUDA_FP16));

    constant("DNN_TARGET_FPGA", static_cast<long>(cv::dnn::DNN_TARGET_FPGA));

    constant("DNN_TARGET_HDDL", static_cast<long>(cv::dnn::DNN_TARGET_HDDL));

    constant("DNN_TARGET_MYRIAD", static_cast<long>(cv::dnn::DNN_TARGET_MYRIAD));

    constant("DNN_TARGET_NPU", static_cast<long>(cv::dnn::DNN_TARGET_NPU));

    constant("DNN_TARGET_OPENCL", static_cast<long>(cv::dnn::DNN_TARGET_OPENCL));

    constant("DNN_TARGET_OPENCL_FP16", static_cast<long>(cv::dnn::DNN_TARGET_OPENCL_FP16));

    constant("DNN_TARGET_VULKAN", static_cast<long>(cv::dnn::DNN_TARGET_VULKAN));

    constant("SoftNMSMethod_SOFTNMS_GAUSSIAN", static_cast<long>(cv::dnn::SoftNMSMethod::SOFTNMS_GAUSSIAN));

    constant("SoftNMSMethod_SOFTNMS_LINEAR", static_cast<long>(cv::dnn::SoftNMSMethod::SOFTNMS_LINEAR));

    constant("CALIB_CHECK_COND", static_cast<long>(cv::fisheye::CALIB_CHECK_COND));

    constant("CALIB_FIX_FOCAL_LENGTH", static_cast<long>(cv::fisheye::CALIB_FIX_FOCAL_LENGTH));

    constant("CALIB_FIX_INTRINSIC", static_cast<long>(cv::fisheye::CALIB_FIX_INTRINSIC));

    constant("CALIB_FIX_K1", static_cast<long>(cv::fisheye::CALIB_FIX_K1));

    constant("CALIB_FIX_K2", static_cast<long>(cv::fisheye::CALIB_FIX_K2));

    constant("CALIB_FIX_K3", static_cast<long>(cv::fisheye::CALIB_FIX_K3));

    constant("CALIB_FIX_K4", static_cast<long>(cv::fisheye::CALIB_FIX_K4));

    constant("CALIB_FIX_PRINCIPAL_POINT", static_cast<long>(cv::fisheye::CALIB_FIX_PRINCIPAL_POINT));

    constant("CALIB_FIX_SKEW", static_cast<long>(cv::fisheye::CALIB_FIX_SKEW));

    constant("CALIB_RECOMPUTE_EXTRINSIC", static_cast<long>(cv::fisheye::CALIB_RECOMPUTE_EXTRINSIC));

    constant("CALIB_USE_INTRINSIC_GUESS", static_cast<long>(cv::fisheye::CALIB_USE_INTRINSIC_GUESS));

    constant("CALIB_ZERO_DISPARITY", static_cast<long>(cv::fisheye::CALIB_ZERO_DISPARITY));

    constant("ENUM_LOG_LEVEL_FORCE_INT", static_cast<long>(cv::utils::logging::ENUM_LOG_LEVEL_FORCE_INT));

    constant("LOG_LEVEL_DEBUG", static_cast<long>(cv::utils::logging::LOG_LEVEL_DEBUG));

    constant("LOG_LEVEL_ERROR", static_cast<long>(cv::utils::logging::LOG_LEVEL_ERROR));

    constant("LOG_LEVEL_FATAL", static_cast<long>(cv::utils::logging::LOG_LEVEL_FATAL));

    constant("LOG_LEVEL_INFO", static_cast<long>(cv::utils::logging::LOG_LEVEL_INFO));

    constant("LOG_LEVEL_SILENT", static_cast<long>(cv::utils::logging::LOG_LEVEL_SILENT));

    constant("LOG_LEVEL_VERBOSE", static_cast<long>(cv::utils::logging::LOG_LEVEL_VERBOSE));

    constant("LOG_LEVEL_WARNING", static_cast<long>(cv::utils::logging::LOG_LEVEL_WARNING));

}
