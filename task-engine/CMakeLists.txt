project(task-engine)

add_library(task-engine STATIC include/task/task.h src/task.cc include/task/task_engine.h src/task_engine.cc src/task_engine_worker.cc)

target_include_directories(task-engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)