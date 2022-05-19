/**
 * @file rev1_direction_module.ino
 * 
 * @author Joseph Telaak
 * 
 * @brief Code for the simple motor controller module
 * 
 * @version 0.1
 * @date 2022-02-12
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "mcp2515.h"
#include "Encoder.h"

// Steering Motor Ctrl
#define STR_L_PWM 6
#define STR_L_ENABLE 4

#define STR_R_PWM 9
#define STR_R_ENABLE 0

// Steering Linear Actuator Potentiometer
#define STR_POT A5
#define STEER_TOL 10 // Tolerance in Increments of ADC's pot value (0-1023)

// Steering Wheel Input Encoder
#define STR_WHL_ENC 3
#define STR_WHL_ENC2 A3

// Brake Motor Ctrl
#define BRK_L_PWM 5
#define BRK_L_ENABLE 7

#define BRK_R_PWM 10
#define BRK_R_ENABLE 1

// Brake Actuator Potentiometer
#define BRK_POT A4

// LEDS
#define COM_LED A0
#define ACT_LED A1

// CAN
#define CAN_CS 8
#define CAN_INT 2
#define CAN_DLC 8
#define CAN_ID 0x001

// CAN Adapter
MCP2515 can(CAN_CS);

// Encoder
Encoder wheel_enc(STR_WHL_ENC, STR_WHL_ENC2);

/**
 * @brief Setup function
 * 
 */

void setup() {
    // Setup LEDS
    pinMode(ACT_LED, OUTPUT);
    pinMode(COM_LED, OUTPUT);

    // Init Hold and Display
    digitalWrite(ACT_LED, HIGH);
    digitalWrite(COM_LED, HIGH);
    delay(1000);
    digitalWrite(ACT_LED, HIGH);
    digitalWrite(COM_LED, LOW);
    delay(200);

    // Setup
    digitalWrite(ACT_LED, HIGH);

    // CAN Setup
    can.reset();
    can.setBitrate(CAN_125KBPS);
    can.setNormalMode();
  
    // Setup Steering Motor
    pinMode(STR_L_ENABLE, OUTPUT);
    pinMode(STR_R_ENABLE, OUTPUT);
    pinMode(STR_L_PWM, OUTPUT);
    pinMode(STR_R_PWM, OUTPUT);
    pinMode(STR_POT, INPUT);

    // Disable
    digitalWrite(STR_L_ENABLE, LOW);
    digitalWrite(STR_R_ENABLE, LOW);
    analogWrite(STR_L_PWM, 0);
    analogWrite(STR_R_PWM, 0);

    // Setup brake motor
    pinMode(BRK_L_ENABLE, OUTPUT);
    pinMode(BRK_R_ENABLE, OUTPUT);
    pinMode(BRK_L_PWM, OUTPUT);
    pinMode(BRK_R_PWM, OUTPUT);
    pinMode(BRK_POT, INPUT);

    // Disable
    digitalWrite(BRK_L_ENABLE, LOW);
    digitalWrite(BRK_R_ENABLE, LOW);
    analogWrite(BRK_L_PWM, 0);
    analogWrite(BRK_R_PWM, 0);

    // LED Low
    digitalWrite(ACT_LED, LOW);

    // CAN Interrupt
    attachInterrupt(digitalPinToInterrupt(CAN_INT), can_irq, FALLING);

}

/**
 * @brief Periodic updates
 * 
 */

void loop() {
    // Updates
    digitalWrite(ACT_LED, HIGH);
    read_brk_pot();
    read_brk_state();
    read_str_pot();
    read_str_state();
    read_str_whl();
    digitalWrite(ACT_LED, LOW);

    // 5 Second Delay
    delay(5000);

}

/**
 * @brief CAN Interrupt
 * 
 */

void can_irq() {
    // Message
    struct can_frame can_msg_in;

    // Check message
    if (can.readMessage(&can_msg_in) == MCP2515::ERROR_OK) {
        digitalWrite(ACT_LED, HIGH);

        // Check ID
        if (can_msg_in.can_id == CAN_ID) {

            if (can_msg_in.data[0] == 0x0A) {
                if (can_msg_in.data[1] == 0x01) {
                    if (can_msg_in.data[2] == 0x0A) {
                        if (can_msg_in.data[3] == 0x01) {
                            digitalWrite(STR_L_ENABLE, LOW);
                            analogWrite(STR_L_PWM, 0);
                            
                            digitalWrite(STR_L_ENABLE, LOW);
                            analogWrite(STR_R_PWM, 0);

                        } else {
                            read_str_state();

                        }

                    } else if (can_msg_in.data[2] == 0x0C) {
                        if (can_msg_in.data[3] == 0x01) {
                            digitalWrite(STR_R_ENABLE, LOW);
                            analogWrite(STR_R_PWM, 0);

                            analogWrite(STR_L_PWM, can_msg_in.data[4]);
                            digitalWrite(STR_L_ENABLE, HIGH);

                        } else if (can_msg_in.data[3] == 0x02) {
                            digitalWrite(STR_L_ENABLE, 0);
                            analogWrite(STR_L_PWM, 0);

                            analogWrite(STR_R_PWM, can_msg_in.data[4]);
                            digitalWrite(STR_R_ENABLE, HIGH);

                        } else {
                            read_str_pot();

                        }
                    }

                } else if (can_msg_in.data[1] == 0x02) {
                    if (can_msg_in.data[2] == 0x0A) {
                        if (can_msg_in.data[3] == 0x01) {
                            digitalWrite(BRK_L_ENABLE, LOW);
                            analogWrite(BRK_L_PWM, 0);

                            digitalWrite(BRK_R_ENABLE, LOW);
                            analogWrite(BRK_R_PWM, 0);

                        } else {
                            read_brk_state();

                        }

                    } else if (can_msg_in.data[2] == 0x0C) {
                        if (can_msg_in.data[3] == 0x01) {
                            digitalWrite(BRK_R_ENABLE, LOW);
                            analogWrite(BRK_R_PWM, 0);

                            analogWrite(BRK_L_PWM, can_msg_in.data[4]);
                            digitalWrite(BRK_L_ENABLE, HIGH);

                        } else if (can_msg_in.data[3] == 0x02) {
                            digitalWrite(BRK_L_ENABLE, LOW);
                            analogWrite(BRK_L_PWM, 0);

                            analogWrite(BRK_R_PWM, can_msg_in.data[4]);
                            digitalWrite(BRK_R_ENABLE, HIGH);

                        } else {
                            read_brk_pot();

                        }
                    }
                }

            } else if (can_msg_in.data[0] == 0x0B) {
                if (can_msg_in.data[1] == 0x01) 
                    steer_to_pos(can_msg_in.data[2] | (can_msg_in.data[3] << 8), can_msg_in.data[3] != 0 ? 255 : can_msg_in.data[4]);
              
            } else if (can_msg_in.data[0] == 0x0C) {
                if (can_msg_in.data[1] == 0x01) {
                    if (can_msg_in.data[2] == 0x0A)
                        read_str_state();
                    else if (can_msg_in.data[3] == 0x0E) 
                        read_str_whl();
                    else if (can_msg_in.data[3] == 0x0F)
                        read_str_pot();

            
                } else if (can_msg_in.data[2] == 0x02) {
                    if (can_msg_in.data[2] == 0x0A) 
                        read_brk_state();
                    else if (can_msg_in.data[2] == 0x0F) 
                        read_brk_pot();

                }
            }
        }

        // Clear the message buffer
        fill_data(&can_msg_in, 0, 7);

        digitalWrite(ACT_LED, LOW);

    }
}

/**
 * @brief Fill the rest of the data frame with zeroes
 * 
 * @param frame Pointer to can frame
 * @param start Start index (inclusive)
 * @param end End index (inclusive)
 */

void fill_data(can_frame* frame, uint8_t start, uint8_t end) {
    for (int i = start; i < end+1; i++) {
        frame->data[i] = 0;

    }
}

/**
 * @brief Read the brake potentiometer
 * 
 */

int read_brk_pot() {
    digitalWrite(COM_LED, HIGH);

    int pot_value = map(analogRead(BRK_POT), 0, 1023, 0, 255);

    struct can_frame can_msg_out;

    can_msg_out.can_id = CAN_ID;
    can_msg_out.can_dlc = CAN_DLC;
    can_msg_out.data[0] = 0x0C;
    can_msg_out.data[1] = 0x0C;
    can_msg_out.data[2] = 0x01;
    can_msg_out.data[3] = 0x0F;
    fill_data(&can_msg_out, 4, 6);
    can_msg_out.data[7] = pot_value;

    can.sendMessage(&can_msg_out);
    digitalWrite(COM_LED, LOW);

    return pot_value;

}

/**
 * @brief Read the brake enable state
 * 
 */

void read_brk_state() {
    digitalWrite(COM_LED, HIGH);
    
    struct can_frame can_msg_out;

    can_msg_out.can_id = CAN_ID;
    can_msg_out.can_dlc = CAN_DLC;
    can_msg_out.data[0] = 0x0C;
    can_msg_out.data[1] = 0x0C;
    can_msg_out.data[2] = 0x02;
    can_msg_out.data[3] = 0x0A;
    fill_data(&can_msg_out, 4, 6);
    can_msg_out.data[7] = (digitalRead(BRK_L_ENABLE) + digitalRead(BRK_R_ENABLE)) + 0x01;
    
    can.sendMessage(&can_msg_out);
    digitalWrite(COM_LED, LOW);

}

/**
 * @brief Steer to a set postion
 * 
 * @param pos Position (0-1023)
 * @param power Power Level (0-255)
 */

void steer_to_pos(int pos, int power) {
    if ((digitalRead(STR_L_ENABLE) + digitalRead(STR_R_ENABLE)) != 0) { return; }
    int current_pos = read_str_pot();

    while (abs(current_pos - pos) > STEER_TOL) {
        if (current_pos < pos) {
            digitalWrite(STR_R_ENABLE, LOW);
            analogWrite(STR_R_PWM, 0);

            analogWrite(STR_L_PWM, power);
            digitalWrite(STR_L_ENABLE, HIGH);

        } else if (current_pos > pos) {
            digitalWrite(STR_R_ENABLE, HIGH);
            analogWrite(STR_R_PWM, power);

            analogWrite(STR_L_PWM, 0);
            digitalWrite(STR_L_ENABLE, HIGH);

        }

        current_pos = read_str_pot();
    }

    read_str_pot();
}

/**
 * @brief Read the steering position
 * 
 */

int read_str_pot() {
    digitalWrite(COM_LED, HIGH);

    int pot_value = map(analogRead(STR_POT), 0, 1023, 0, 255);

    struct can_frame can_msg_out;

    can_msg_out.can_id = CAN_ID;
    can_msg_out.can_dlc = CAN_DLC;
    can_msg_out.data[0] = 0x0C;
    can_msg_out.data[1] = 0x0C;
    can_msg_out.data[2] = 0x01;
    can_msg_out.data[3] = 0x0F;
    fill_data(&can_msg_out, 4, 6);;
    can_msg_out.data[7] = pot_value;
    
    can.sendMessage(&can_msg_out);
    digitalWrite(COM_LED, LOW);

    return pot_value;

}

/**
 * @brief Read the steering enable state
 * 
 */

void read_str_state() {
    digitalWrite(COM_LED, HIGH);

    struct can_frame can_msg_out;

    can_msg_out.can_id = CAN_ID;
    can_msg_out.can_dlc = CAN_DLC;
    can_msg_out.data[0] = 0x0C;
    can_msg_out.data[1] = 0x0C;
    can_msg_out.data[2] = 0x01;
    can_msg_out.data[3] = 0x0A;
    fill_data(&can_msg_out, 4, 6);
    can_msg_out.data[7] = (digitalRead(STR_L_ENABLE) + digitalRead(STR_R_ENABLE)) + 0x01;

    can.sendMessage(&can_msg_out);
    digitalWrite(COM_LED, LOW);

}

long old_pos = -999;

/**
 * @brief Read the steering wheel change
 * 
 */

long read_str_whl() {
    digitalWrite(COM_LED, HIGH);

    long pos = wheel_enc.read();
    uint8_t change_id = 0x01;

    if (pos > old_pos)
        change_id = 0x02;
    else if (pos < old_pos)
        change_id = 0x03;

    struct can_frame can_msg_out;

    can_msg_out.can_id = CAN_ID;
    can_msg_out.can_dlc = CAN_DLC;
    can_msg_out.data[0] = 0x0C;
    can_msg_out.data[1] = 0x0C;
    can_msg_out.data[2] = 0x01;
    can_msg_out.data[3] = 0x0E;
    fill_data(&can_msg_out, 4, 6);
    can_msg_out.data[7] = change_id;

    can.sendMessage(&can_msg_out);
    digitalWrite(COM_LED, LOW);

    return pos;

}
