project(tkn-task-engine)

add_library(tkn-task-engine include/task/task.h src/task.cc include/task/task_engine.h src/task_engine.cc src/task_engine_worker.cc)
target_include_directories(tkn-task-engine PUBLIC ${PROJECT_SOURCE_DIR}/include)