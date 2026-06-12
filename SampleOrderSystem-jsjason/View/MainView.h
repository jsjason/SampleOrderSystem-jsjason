#pragma once

class MainView {
public:
    void showMenu(int sampleCount, int totalStock) const;
    int  promptMenuChoice() const;
    void showNotImplemented() const;
    void showInvalidInput() const;
};
