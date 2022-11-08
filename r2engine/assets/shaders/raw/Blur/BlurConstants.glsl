#ifdef HORIZONTAL_WEIGHTS
//horizontal offsets
const ivec2 offsets[7] = {{-3, 0}, {-2, 0}, {-1, 0}, {0, 0}, {1, 0}, {2, 0}, {3, 0}};

const float weights[7] = {0.001f, 0.028f, 0.233f, 0.474f, 0.233f, 0.028f, 0.001f};

#else
//vertical offsets
const ivec2 offsets[7] = {{0, -3}, {0, -2}, {0, -1}, {0, 0}, {0, 1}, {0, 2}, {0, 3}};

const float weights[7] = {0.001f, 0.028f, 0.233f, 0.474f, 0.233f, 0.028f, 0.001f};

#endif

const int TEST_CONST = 5;