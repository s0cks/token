project(tkn-task-engine)

add_library(tkn-task-engine STATIC include/task/task.h src/task/task.cc include/task/task_queue.h include/task/task_debug.h include/task/task_engine.h src/task/task_engine.cc include/task/task_engine_worker.h src/task/task_engine_worker.cc)
target_link_libraries(tkn-task-engine tkn-platform tkn-utils)
target_include_directories(tkn-task-engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)