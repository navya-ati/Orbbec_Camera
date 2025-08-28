#include <iostream>
#include <libobsensor/ObSensor.hpp>
#include <opencv2/opencv.hpp>
#include <cstdlib>  
#include <unistd.h>
#include <csignal>

// Signal handler: stops program immediately when Ctrl+C is pressed
void signalHandler(int signum) {
    std::cout << "\nCtrl+C pressed. Stopping program immediately..." << std::endl;
    exit(0);  // terminate program immediately
}

int main() {
    try {
        // 1. Set up Ctrl+C handler
        std::signal(SIGINT, signalHandler);

        // 2. Define file paths
        std::string baseDir = std::string(getenv("HOME")) + "/OrbbecCamera/build/projects/";
        std::string obsFile = baseDir + "record_video.obs"; // input file
        std::string mp4File = baseDir + "record_video.mp4"; // output file

        // 3. Create playback device for the recorded .obs file
        auto playbackDev = std::make_shared<ob::PlaybackDevice>(obsFile);
        ob::Pipeline pipe(playbackDev);

        // 4. Configure pipeline to enable only color stream
        auto config = std::make_shared<ob::Config>();
        config->enableStream(OB_STREAM_COLOR);

        // 5. Set up OpenCV VideoWriter to save .mp4
        cv::VideoWriter writer(mp4File,
                               cv::VideoWriter::fourcc('a','v','c','1'), // codec
                               15, // FPS
                               cv::Size(640, 480)); // resolution
        if (!writer.isOpened()) {
            std::cerr << "Cannot open " << mp4File << std::endl;
            return -1;
        }

        // 6. Start pipeline and write frames to MP4 in callback
        pipe.start(config, [&](std::shared_ptr<ob::FrameSet> frames) {
            auto colorFrameRaw = frames->getFrame(OB_FRAME_COLOR);
            if (!colorFrameRaw) return;

            auto colorFrame = colorFrameRaw->as<ob::ColorFrame>();
            cv::Mat img(colorFrame->getHeight(),
                        colorFrame->getWidth(),
                        CV_8UC3,
                        (void*)colorFrame->getData());
            cv::cvtColor(img, img, cv::COLOR_RGB2BGR); 
            writer.write(img);
        });

        std::cout << "Conversion started: .obs -> .mp4" << std::endl;

        // 7. Keep main thread alive (will exit on Ctrl+C)
        while (true) {
            usleep(1000000); // wait 1 second
        }

        // 8. Stop pipeline and release VideoWriter (will not reach here on Ctrl+C)
        pipe.stop();
        writer.release();

        std::cout << "Conversion finished. Output: " << mp4File << std::endl;

    } catch (ob::Error &e) {
        std::cerr << "Orbbec SDK error: " << e.getMessage() << std::endl;
        return -1;
    }

    return 0;
}
