#pragma once

#include "ToolboxUIElement.h"

class Updater : public ToolboxUIElement {
	Updater() {};
	~Updater() {};
public:
	static Updater& Instance() {
		static Updater instance;
		return instance;
	}
    struct GWToolboxRelease {
        std::string body;
        std::string version;
        std::string download_url;
    };
	const char* Name() const override { return "Updater"; }

	void CheckForUpdate(const bool forced = false);
	void DoUpdate();

	void Draw(IDirect3DDevice9* device) override;

	void LoadSettings(CSimpleIni* ini) override;
	void SaveSettings(CSimpleIni* ini) override;
	void Initialize() override;
	void DrawSettings() override {};
	void DrawSettingInternal() override;
    void GetLatestRelease(GWToolboxRelease*);
	std::string GetServerVersion() const { return latest_release.version; }

private:
    GWToolboxRelease latest_release;

	// 0=none, 1=check and warn, 2=check and ask, 3=check and do
	int mode = 2;

	// 0=checking, 1=asking, 2=downloading, 3=done
	enum Step {
		Checking,
		Asking,
		Downloading,
		Success,
		Done
	};
	bool notified = false;
	bool forced_ask = false;
	clock_t last_check = 0;

	Step step = Checking;
};
