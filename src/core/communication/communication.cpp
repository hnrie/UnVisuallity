//
// Created by user on 21/04/2025.
//

#include <fstream>
#include <thread>
#include "communication.h"
#include "src/rbx/taskscheduler/taskscheduler.h"

void communication::start() {
    auto scheduler_path = std::filesystem::path(getenv("localappdata")) / "Visual" / "scheduler";
    if (!std::filesystem::exists(scheduler_path))
        std::filesystem::create_directory(scheduler_path);

    //std::filesystem::remove_all(scheduler_path); // dont exec scripts from last session

    while (true) {
        for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(scheduler_path)) {
            if (std::filesystem::exists(entry) && std::filesystem::is_regular_file(entry)) {
                const std::filesystem::path& file_path = entry.path();
                std::ifstream file_stream(file_path, std::ios::binary);

                if (file_stream.is_open()) {
                    std::string file_content;
                    std::copy(std::istreambuf_iterator< char >(file_stream), std::istreambuf_iterator< char >(), std::back_inserter(file_content));
                    file_stream.close();

                    bool is_ready = file_content.contains("@@FINISHED@@");

                    if (is_ready) {
                        size_t pos;

                        while ((pos = file_content.find("@@FINISHED@@")) != std::string::npos) {
                            file_content.erase(pos, 12);
                        }

                        g_taskscheduler->queue_script(file_content);

                        try {
                            std::filesystem::remove(entry.path());
                        }
                        catch (std::exception& e) { }
                    }
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }
}

