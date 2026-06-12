#pragma once

class MainView {
public:
    void showMenu(int sampleCount, int totalStock,
                  int orderCount,  int queueCount) const;
    int  promptMenuChoice() const;
    void showNotImplemented() const;
    void showInvalidInput() const;
};
