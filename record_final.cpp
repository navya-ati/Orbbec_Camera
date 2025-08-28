#include <iostream>
#include <libobsensor/ObSensor.hpp>
#include <unistd.h>
#include <csignal>
#include <cstdlib>  // For getenv

bool stopRecording = false;

// Function to handle Ctrl+C to stop recording
void signalHandler(int signum) {
    std::cout << "\nCtrl+C pressed. Stopping recording..." << std::endl;
    stopRecording = true;
}

int main() {
    try {
        // 1. Set up Ctrl+C handler
        std::signal(SIGINT, signalHandler);

        // 2. Create context and check for connected Orbbec device
        ob::Context ctx;
        auto devList = ctx.queryDeviceList();
        if (devList->deviceCount() == 0) {
            std::cerr << "No Orbbec device found!" << std::endl;
            return -1;
        }
        auto dev = devList->getDevice(0); // Use the first connected device

        // 3. Set the output file path 
        std::string baseDir = std::string(getenv("HOME")) + "/OrbbecCamera/build/projects/";
        std::string filePath = baseDir + "record_video.obs";

        // 4. Create a RecordDevice to save recorded data
        auto recordDev = std::make_shared<ob::RecordDevice>(dev, filePath.c_str());

        // 5. Create a pipeline to get frames from the camera
        ob::Pipeline pipe(dev);

        // 6. Configure which streams to record (Depth + Color)
        auto config = std::make_shared<ob::Config>();
        config->enableVideoStream(OB_STREAM_DEPTH, 640, 400, 15, OB_FORMAT_Y16);
        config->enableVideoStream(OB_STREAM_COLOR, 640, 480, 15, OB_FORMAT_RGB888);

        // 7. Start the pipeline and process frames in a callback
        pipe.start(config, [&](std::shared_ptr<ob::FrameSet> frameSet) {
            auto depthFrameRaw = frameSet->getFrame(OB_FRAME_DEPTH);
            auto colorFrameRaw = frameSet->getFrame(OB_FRAME_COLOR);

            if (depthFrameRaw) {
                auto depthFrame = depthFrameRaw->as<ob::DepthFrame>();
                std::cout << "Depth frame: " << depthFrame->getWidth() << "x" << depthFrame->getHeight() << std::endl;
            }

            if (colorFrameRaw) {
                auto colorFrame = colorFrameRaw->as<ob::ColorFrame>();
                std::cout << "Color frame: " << colorFrame->getWidth() << "x" << colorFrame->getHeight() << std::endl;
            }
        });

        std::cout << "Recording started. Saving to " << filePath << std::endl;
        std::cout << "Press Ctrl+C to stop..." << std::endl;

        // 8. Keep running until Ctrl+C is pressed
        while (!stopRecording) {
            sleep(1); // Wait for 1 second in each loop
        }

        // 9. Stop the pipeline and finish recording
        pipe.stop();
        std::cout << "Recording stopped." << std::endl;

    } catch (ob::Error &e) {
        std::cerr << "Orbbec SDK error: " << e.getMessage() << std::endl;
        return -1;
    }

    return 0;
}
