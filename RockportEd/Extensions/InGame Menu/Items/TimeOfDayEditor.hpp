#pragma once
#include "stdafx.h"
#include "Extensions\Extensions.h"
using GameInternals::TimeOfDay;
using GameInternals::TimeOfDayLighting_WithOptionalParameters;
// ImGui::VerticalSeparator
#include "Helpers\imgui\imgui_internal.h"

namespace Extensions {
   namespace InGameMenu {
      class TimeOfDayEditor : public _BaseInGameMenuItem {
         int32_t currentLightingIndex = 0;
         std::vector<std::string> lightingHashes = std::vector<std::string>();
         std::map<int32_t, TimeOfDayLighting_WithOptionalParameters*> lightingDefinitions = {};

         TimeOfDay*                                todInstance             = nullptr;
         TimeOfDayLighting_WithOptionalParameters* todLightingEditInstance = nullptr;

      public:
         const virtual void loadData() override {
            const DWORD* todLightingsArray = nullptr;
            while (!todLightingsArray) {
               todLightingsArray = Memory::readPointer(0x5B392C, 3, 0xEC, 0x14, 0x1C);
               Sleep(10);
            }
            todInstance = *reinterpret_cast<TimeOfDay**>(0x5B392C + 0x400000);

            int step = 0;
            int lightingIndex = 0;
            while (true) {
               DWORD hash = *(DWORD*)(*todLightingsArray + step);
               if (hash == 0xAAAAAAAA) {
                  break;
               } else if (hash) {
                  char* strHash = new char[10 + 1];
                  sprintf_s(strHash, 10 + 1, "0x%08X", hash);
                  std::string strHash_proper(strHash);
                  lightingHashes.push_back(strHash_proper);
                  delete[] strHash;

                  lightingDefinitions[lightingIndex] = *reinterpret_cast<TimeOfDayLighting_WithOptionalParameters**>(*todLightingsArray + step + 0x4);
                  if (Settings::settingsType.todPresets.size() != 0) {
                     auto iter = Settings::settingsType.todPresets.find(strHash_proper);
                     if (iter != Settings::settingsType.todPresets.end()) {
                        auto* gameInstance     = lightingDefinitions[lightingIndex];
                        auto* settingsInstance = &iter->second;

                        memcpy_s(gameInstance->pLightingData->SpecularColour, sizeof(float) * 4, settingsInstance->SpecularColour, sizeof(float) * 4);
                        memcpy_s(gameInstance->pLightingData->DiffuseColour, sizeof(float) * 4, settingsInstance->DiffuseColour, sizeof(float) * 4);
                        memcpy_s(gameInstance->pLightingData->AmbientColour, sizeof(float) * 4, settingsInstance->AmbientColour, sizeof(float) * 4);
                        memcpy_s(gameInstance->pLightingData->FogSkyColour, sizeof(float) * 4, settingsInstance->FogSkyColour, sizeof(float) * 4);
                        memcpy_s(gameInstance->pLightingData->FogHazeColour, sizeof(float) * 4, settingsInstance->FogHazeColour, sizeof(float) * 4);
                        memcpy_s(gameInstance->pLightingData->FixedFunctionSkyColour, sizeof(float) * 4, settingsInstance->FixedFunctionSkyColour, sizeof(float) * 4);
                        gameInstance->pLightingData->FogDistanceScale   = settingsInstance->FogDistanceScale;
                        gameInstance->pLightingData->FogSkyColourScale  = settingsInstance->FogSkyColourScale;
                        gameInstance->pLightingData->FogHazeColourScale = settingsInstance->FogHazeColourScale;
                        gameInstance->pLightingData->EnvSkyBrightness   = settingsInstance->EnvSkyBrightness;
                        gameInstance->pLightingData->CarSpecScale       = settingsInstance->CarSpecScale;
                        gameInstance->FogInLightScatter                 = settingsInstance->FogInLightScatter;
                        gameInstance->FogSunFalloff                     = settingsInstance->FogSunFalloff;
                     }
                  }

                  if (todInstance->pTimeOfDayLightingInstanceWrapper == lightingDefinitions[lightingIndex])
                     currentLightingIndex = lightingIndex;
                  lightingIndex++;
               }

               step += 0xC;
               Sleep(1);
            }

            bool* TimeOfDaySwapEnable = (bool*)(0x4F86E8 + 0x400000);
            Memory::writeRaw((DWORD)TimeOfDaySwapEnable, true, 1, 0x0);
            hasLoadedData = true;
         }

         const virtual void onFrame() override {}

         const virtual bool displayMenuItem(const ImVec2& buttonSize) override {
            return ImGui::Button("Time of day and lighting editor", buttonSize);
         }
         const virtual bool displayMenu() override {
            static float lineDiff = 0.0f;
            lineDiff = ImGui::CalcTextSize("Time of day progression speed multiplier...").x + ImGui::GetStyle().WindowPadding.x;
            ImGui::PushItemWidth(lineDiff * 0.625f);

            ImGui::Text("Skybox animation speed multiplier"); ImGui::SameLine(lineDiff);
            ImGui::SliderFloat("##SkySpeedMult", &todInstance->SkyboxSpeedMultiplier, -100.0f, 100.0f);

            ImGui::Text("Time of day progression speed multiplier"); ImGui::SameLine(lineDiff);
            ImGui::SliderInt("##ToDSpeedMult", &todInstance->TimeOfDaySpeedMultiplier, -100, 100);

            ImGui::Text("Time of day"); ImGui::SameLine(lineDiff);
            ImGui::SliderFloat("##ToDValue", &todInstance->TimeOfDayValue, 0.05f, 0.95f);

            ImGui::Text("Sun default orbit X-Axis (Horizontal)"); ImGui::SameLine(lineDiff);
            ImGui::SliderAngle("##SunOrbitXAxis", &todInstance->SunDefaultOrbitAxisX);

            ImGui::Text("Sun default orbit Y-Axis (Vertical)"); ImGui::SameLine(lineDiff);
            ImGui::SliderAngle("##SunOrbitYAxis", &todInstance->SunDefaultOrbitAxisY);
            ImGui::PopItemWidth();
            ImGui::Separator();

            if (todLightingEditInstance) {
               ImGui::Text("Lighting hash "); ImGui::SameLine();
               if (ImGui::Combo("##CurLighting", &currentLightingIndex, lightingHashes)) {
                  todLightingEditInstance = lightingDefinitions[currentLightingIndex];
               }
               ImGui::SameLine();
               if (ImGui::Button("Set active")) {
                  todInstance->pTimeOfDayLightingInstanceWrapper = lightingDefinitions[currentLightingIndex];
                  todInstance->pTimeOfDayLightingInstance        = todInstance->pTimeOfDayLightingInstanceWrapper->pLightingData;
                  todInstance->TimeOfDayValue                    = 0.5f;
               }

               if (ImGui::Button("Save preset")) {
                  Settings::TimeOfDayLightingPreset* presetInstance = &Settings::settingsType.todPresets[lightingHashes[currentLightingIndex]];
                  presetInstance->FogInLightScatter = todLightingEditInstance->FogInLightScatter;
                  presetInstance->FogSunFalloff     = todLightingEditInstance->FogSunFalloff;
                  *presetInstance                   = todLightingEditInstance->pLightingData;
                  Settings::saveSettings();
               }
               ImGui::SameLine(); ImGui::VerticalSeparator(); ImGui::SameLine();
               if (ImGui::Button("Save preset for all")) {
                  for (auto& lightingDef : lightingDefinitions) {
                     // save
                     Settings::TimeOfDayLightingPreset* presetInstance = &Settings::settingsType.todPresets[lightingHashes[lightingDef.first]];
                     presetInstance->FogInLightScatter = todLightingEditInstance->FogInLightScatter;
                     presetInstance->FogSunFalloff     = todLightingEditInstance->FogSunFalloff;
                     *presetInstance                   = todLightingEditInstance->pLightingData;

                     // apply
                     auto* listTimeOfDayMemoryInstance = lightingDef.second;
                     listTimeOfDayMemoryInstance->FogInLightScatter = todLightingEditInstance->FogInLightScatter;
                     listTimeOfDayMemoryInstance->FogSunFalloff     = todLightingEditInstance->FogSunFalloff;
                     *listTimeOfDayMemoryInstance->pLightingData    = *todLightingEditInstance->pLightingData;
                  }
                  Settings::saveSettings();
               }

               lineDiff = ImGui::CalcTextSize("FixedFunctionSkyColour...").x + ImGui::GetStyle().WindowPadding.x;
               ImGui::PushItemWidth(-1.0f);

               ImGui::Text("SpecularColour"); ImGui::SameLine(lineDiff);
               ImGui::ColorEdit4("##SpecularColour", todLightingEditInstance->pLightingData->SpecularColour);

               ImGui::Text("DiffuseColour"); ImGui::SameLine(lineDiff);
               ImGui::ColorEdit4("##DiffuseColour", todLightingEditInstance->pLightingData->DiffuseColour);

               ImGui::Text("AmbientColour"); ImGui::SameLine(lineDiff);
               ImGui::ColorEdit4("##AmbientColour", todLightingEditInstance->pLightingData->AmbientColour);

               ImGui::Text("FogHazeColour"); ImGui::SameLine(lineDiff);
               ImGui::ColorEdit4("##FogHazeColour", todLightingEditInstance->pLightingData->FogHazeColour);

               ImGui::Text("FixedFunctionSkyColour"); ImGui::SameLine(lineDiff);
               ImGui::ColorEdit4("##FixedFunctionSkyColour", todLightingEditInstance->pLightingData->FixedFunctionSkyColour);

               ImGui::Text("FogDistanceScale"); ImGui::SameLine(lineDiff);
               ImGui::DragFloat("##FogDistanceScale", &todLightingEditInstance->pLightingData->FogDistanceScale, 10.0f, -100.0f, 1000.0f);

               ImGui::Text("FogHazeColourScale"); ImGui::SameLine(lineDiff);
               ImGui::DragFloat("##FogHazeColourScale", &todLightingEditInstance->pLightingData->FogHazeColourScale, 10.0f, 0.0f, 1000.0f);

               ImGui::Text("FogSkyColourScale"); ImGui::SameLine(lineDiff);
               ImGui::DragFloat("##FogSkyColourScale", &todLightingEditInstance->pLightingData->FogSkyColourScale, 10.0f, 0.0f, 1000.0f);

               ImGui::Text("EnvSkyBrightness"); ImGui::SameLine(lineDiff);
               ImGui::SliderFloat("##EnvSkyBrightness", &todLightingEditInstance->pLightingData->EnvSkyBrightness, 0.0f, 10.0f);

               ImGui::Text("CarSpecScale"); ImGui::SameLine(lineDiff);
               ImGui::DragFloat("##CarSpecScale", &todLightingEditInstance->pLightingData->CarSpecScale, 0.25f, 0.0f, 100.0f);

               ImGui::Text("FogSkyColour"); ImGui::SameLine(lineDiff);
               ImGui::ColorEdit4("##FogSkyColour", todLightingEditInstance->pLightingData->FogSkyColour);

               ImGui::Text("FogInLightScatter"); ImGui::SameLine(lineDiff);
               ImGui::SliderFloat("##FogInLightScatter", &todLightingEditInstance->FogInLightScatter, -100.0f, 100.0f);

               ImGui::Text("FogSunFalloff"); ImGui::SameLine(lineDiff);
               ImGui::SliderFloat("##FogSunFalloff", &todLightingEditInstance->FogSunFalloff, -3.0f, 3.0f);

               ImGui::PopItemWidth();
            } else {
               todLightingEditInstance = lightingDefinitions[currentLightingIndex];
               ImGui::Text("There was an issue...");
            }
            return true;
         }
      };
   }
}