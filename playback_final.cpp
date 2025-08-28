#include <iostream>
#include <libobsensor/ObSensor.hpp>
#include <opencv2/opencv.hpp>
#include <unistd.h>
#include <cstdlib> 

int main() {
    try {
        // 1. Set the path to the recorded .obs file 
        std::string baseDir = std::string(getenv("HOME")) + "/OrbbecCamera/build/projects/";
        std::string filePath = baseDir + "record_video.obs";

        // 2. Create a playback device to read the recorded file
        auto playbackDev = std::make_shared<ob::PlaybackDevice>(filePath);

        // 3. Create a pipeline using the playback device
        ob::Pipeline pipe(playbackDev);

        // 4. Configure the pipeline to enable all streams available in the playback
        auto config = std::make_shared<ob::Config>();
        auto sensorList = playbackDev->getSensorList();
        for (uint32_t i = 0; i < sensorList->getCount(); i++) {
            auto sensorType = sensorList->getSensorType(i);
            config->enableStream(sensorType);
        }

        // 5. Start the pipeline and handle frames in a callback function
        pipe.start(config, [&](std::shared_ptr<ob::FrameSet> frameSet) {
            auto depthFrameRaw = frameSet->getFrame(OB_FRAME_DEPTH);
            auto colorFrameRaw = frameSet->getFrame(OB_FRAME_COLOR);

            // 5a. Display depth frame
            if (depthFrameRaw) {
                auto depthFrame = depthFrameRaw->as<ob::DepthFrame>();
                uint32_t width = depthFrame->getWidth();
                uint32_t height = depthFrame->getHeight();
                const uint16_t* depthData = reinterpret_cast<const uint16_t*>(depthFrame->getData());

                // Convert depth to 8-bit for visualization
                cv::Mat depthMat(height, width, CV_16UC1, (void*)depthData);
                cv::Mat depth8u;
                depthMat.convertTo(depth8u, CV_8UC1, 255.0 / 10000); // scale depth values to 0-255
                cv::applyColorMap(depth8u, depth8u, cv::COLORMAP_JET);

                cv::imshow("Depth", depth8u);
            }

            // 5b. Display color frame
            if (colorFrameRaw) {
                auto colorFrame = colorFrameRaw->as<ob::ColorFrame>();
                uint32_t width = colorFrame->getWidth();
                uint32_t height = colorFrame->getHeight();
                const uint8_t* colorData = reinterpret_cast<const uint8_t*>(colorFrame->getData());

                cv::Mat colorMat(height, width, CV_8UC3, (void*)colorData);
                cv::cvtColor(colorMat, colorMat, cv::COLOR_RGB2BGR); // OpenCV uses BGR format
                cv::imshow("Color", colorMat);
            }

            cv::waitKey(1); // Update the OpenCV windows
        });

        std::cout << "Playback started. Press Ctrl+C to stop..." << std::endl;

        // 6. Keep main thread alive so callback keeps running
        while (true) { sleep(1); }

        pipe.stop(); // Stop pipeline (this will never run unless you add signal handling)

    } catch (ob::Error &e) {
        std::cerr << "Orbbec SDK error: " << e.getMessage() << std::endl;
        return -1;
    }

    return 0;
}
