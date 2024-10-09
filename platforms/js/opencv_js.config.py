# Classes and methods whitelist

core = {
    '': [
        'absdiff', 'add', 'addWeighted', 'bitwise_and', 'bitwise_not', 'bitwise_or', 'bitwise_xor', 'cartToPolar',
        'compare', 'convertScaleAbs', 'copyMakeBorder', 'countNonZero', 'determinant', 'dft', 'divide', 'eigen',
        'exp', 'flip', 'getOptimalDFTSize','gemm', 'hconcat', 'inRange', 'invert', 'kmeans', 'log', 'magnitude',
        'max', 'mean', 'meanStdDev', 'merge', 'min', 'minMaxLoc', 'mixChannels', 'multiply', 'norm', 'normalize',
        'perspectiveTransform', 'polarToCart', 'pow', 'randn', 'randu', 'reduce', 'repeat', 'rotate', 'setIdentity', 'setRNGSeed',
        'solve', 'solvePoly', 'split', 'sqrt', 'subtract', 'trace', 'transform', 'transpose', 'vconcat',
        'setLogLevel', 'getLogLevel',
        'LUT',
    ],
    'Algorithm': [],
}

calib3d = {
    '': [
        'findHomography',
        'calibrateCameraExtended',
        'drawFrameAxes',
        'estimateAffine2D',
        'getDefaultNewCameraMatrix',
        'initUndistortRectifyMap',
        'Rodrigues',
        'solvePnP',
        'solvePnPRansac',
        'solvePnPRefineLM',
        'projectPoints',
        'undistort',

        # cv::fisheye namespace
        'fisheye_initUndistortRectifyMap',
        'fisheye_projectPoints',
    ],
}

white_list = makeWhiteList([core, calib3d])

# namespace_prefix_override['dnn'] = ''  # compatibility stuff (enabled by default)
# namespace_prefix_override['aruco'] = ''  # compatibility stuff (enabled by default)
