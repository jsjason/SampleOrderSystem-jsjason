#include <windows.h>

#ifdef _DEBUG
#include <gtest/gtest.h>
#endif

int main(int argc, char* argv[]) {
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

#ifdef _DEBUG
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
#else
    // TODO: AppController 조립 및 실행
    return 0;
#endif
}
