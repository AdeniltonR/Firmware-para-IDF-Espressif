idf_component_register(SRCS "main.c"
                            "./task/servo_control/servo_control.c"
                            "./task/uart_control/uart_control.c"
                    INCLUDE_DIRS "."
                    "./task/servo_control/include"
                    "./task/uart_control/include"
                    )