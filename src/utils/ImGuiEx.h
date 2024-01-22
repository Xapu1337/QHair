#pragma once
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "../libaries/imgui/imgui.h"
struct ExampleAppConsole
{
    char                  InputBuf[256];
    ImVector<char*>       Items;
    ImVector<const char*> Commands;
    ImVector<char*>       History;
    int                   HistoryPos;    // -1: new line, 0..History.Size-1 browsing history.
    ImGuiTextFilter       Filter;
    bool                  AutoScroll;
    bool                  ScrollToBottom;

    ExampleAppConsole();
    ~ExampleAppConsole();

    void ClearLog();
    void AddLog(const char* fmt, ...);
    void Draw(const char* title, bool* p_open);
    void ExecCommand(const char* command_line);

    static int TextEditCallbackStub(ImGuiInputTextCallbackData* data);
    int TextEditCallback(ImGuiInputTextCallbackData* data);
    static void ShowExampleAppConsole(bool* p_open);
};


struct ExampleAppLog
{
    ExampleAppLog();

    void Clear();
    void AddLog(const char* fmt, ...);
    void Draw(const char* title, bool* p_open = nullptr);

    ImGuiTextBuffer Buf;
    ImGuiTextFilter Filter;
    ImVector<int> LineOffsets;
    bool AutoScroll;
    static void ShowExampleAppLog(bool* p_open);
};

