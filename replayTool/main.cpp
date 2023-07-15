#include <chrono>
#include <iostream>
#include <filesystem>
#include <replayTool/debugDraw.hpp>
#include <shared/ReplayData.hpp>
#include <engine/Utils/FileIo.hpp>
#include <engine/Utils/RefOptional.hpp>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_win32.h>
#include <imgui/imgui_impl_opengl3.h>
#include <replayTool/Init.hpp>
#include <raylib/raylib.h>

std::optional<ReplayData> tryLoadReplayData(const char* path) {
	const auto json = tryLoadJsonFromFile(path);
	if (!json.has_value()) {
		std::cout << "failed to read " << path << '\n';
		return std::nullopt;
	}
	const auto replayData = tryLoadFromJson<ReplayData>(*json);
	if (!replayData.has_value()) {
		std::cout << "failed parse json " << path << '\n';
		return std::nullopt;
	}
	return *replayData;
};



std::optional<int> findFrameClosestTo(const ReplayData& replay, float time) {
	const auto& frames = replay.frames;
	auto const it = std::lower_bound(frames.begin(), frames.end(), time, [](const ReplayFrame& frame, float time) {
		return frame.globalClockTime < time;
	});
	if (it == frames.end()) {
		return std::nullopt;
	}
	return it - frames.begin();
}

struct ReplayTool {
	struct Replay {
		std::string name;
		ReplayData replay;
		std::optional<ReplayFrame> currentFrame;
		Color color;
		float timeOffset = 0.0f;
	};
	std::vector<Replay> replays;

	Camera2D camera{
		.offset = Vector2{ 0.0f, 0.0f },
		.target = 0.0f,
		.rotation = 0.0f,
		.zoom = 500.0f,
	};

	float time = 0.0f;
	bool timePaused = false;

	void tryLoadReplay(const char* name, const char* path) {
		auto replay = tryLoadReplayData(path);
		if (!replay.has_value()) {
			return;
		}
		if (replay->frames.size() >= 1) {
			time = replay->frames[0].globalClockTime;
		}
		Color color = WHITE;
		if (replays.size() == 0) {
			color = BLUE;
		} else if (replays.size() == 1) {
			color = GREEN;
		} else if (replays.size() == 2) {
			color = RED;
		}
		replays.push_back(Replay{
			.name = std::move(name),
			.replay = std::move(*replay),
			.color = color
		});
	}

	ReplayTool() {
		tryLoadReplay("client0", "./generated/clientReplay.json0");
		tryLoadReplay("client1", "./generated/clientReplay.json1");
		tryLoadReplay("server", "./generated/serverReplay.json");
		//float scale = 500.0f;

		const auto windowWidth = GetScreenWidth();
		const auto windowHeight = GetScreenHeight();
		camera.offset.x += windowWidth / 2;
		camera.offset.y += windowHeight / 2;
	} 

	void update() {
		ImGui::InputFloat("time", &time);
		ImGui::Checkbox("time paused", &timePaused);
		for (auto& replay : replays) {
			ImGui::PushID(&replay);
			ImGui::Text("%s", replay.name.c_str());
			ImGui::InputFloat("time offset", &replay.timeOffset);
			float color[3]{ replay.color.r / 255.0f, replay.color.g / 255.0f, replay.color.b / 255.0f };
			ImGui::ColorEdit3("color", color);
			ImGui::PopID();
			ImGui::Separator();
		}
		if (!timePaused) {
			time += GetFrameTime();
		}


		for (auto& replay : replays) {
			const auto index = findFrameClosestTo(replay.replay, time + replay.timeOffset);
			if (!index.has_value()) {
				continue;
			}
			replay.currentFrame = replay.replay.frames[*index];
		}

		BeginMode2D(camera);

		for (auto& replay : replays) {
			if (replay.currentFrame.has_value()) {
				ImGui::Text("%g", replay.currentFrame->globalClockTime);
				auto& frame = replay.currentFrame;
				debugDraw(frame->gameplayState, frame->players, replay.color);
			}
		}

		EndMode2D();
	}
};

int main(int argc, char* argv[]) {
	std::cout << std::filesystem::current_path() << '\n';
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(800, 600, "replay tool");
	initImGui(GetWindowHandle());

	SetWindowState(FLAG_WINDOW_MAXIMIZED);
	SetExitKey(-1);

	ReplayTool tool;
	
	while (!WindowShouldClose()) {
		BeginDrawing();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ClearBackground(BLACK);
		tool.update();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		EndDrawing();
	}
	CloseWindow();

	// Raylib causes yojimbo to leak memory for some reason. If this is commented out the leak check doesn't happen.
	//ShutdownYojimbo();

	return EXIT_SUCCESS;
}