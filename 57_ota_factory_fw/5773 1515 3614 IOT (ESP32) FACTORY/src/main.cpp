#include <manager/manager.h>

manager &m = manager::getInstance(); // invoke singleton instance

void print_data()
{
  Serial.println();

  Serial.print("WORK DATA: ");
  for (uint8_t i = 0x00; i < m.work_data_len; i++)
  {
    Serial.print(m.work_data[i]);
    Serial.print(" ");
  }
  Serial.println();

  Serial.print("CAL DATA: ");
  for (uint8_t i = 0x00; i < m.calibration_data_len; i++)
  {
    Serial.print(m.calibration_data[i]);
    Serial.print(" ");
  }
  Serial.println();

  Serial.print("REG DATA: ");
  for (uint8_t i = 0x00; i < m.regulation_data_len; i++)
  {
    Serial.print(m.regulation_data[i]);
    Serial.print(" ");
  }
  Serial.println();
  Serial.println();
}

void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  Serial.println("STARTING");
  Serial.println();

  if (m.begin() != EXIT_SUCCESS)
    esp_restart(); // something went wrong

  Serial.print("ACTUAL CODE VERSION: ");
  Serial.println(m.keys[1].c_str());

  print_data(); // DEBUG ONLY

  delay(3000);
}

void loop()
{
  // if (Serial.available())
  // {
  //   while (Serial.available())
  //     Serial.read();

  //   m.ota.make_update();

  //   uint8_t scode = m.fb.push_timestamp();

  //   while (scode != EXIT_SUCCESS)
  //   {
  //       scode = m.fb.push_timestamp();

  //       static uint8_t aux;
  //       aux++;

  //       if (aux >= m.retry_count)
  //           return;
  //   }
  // }

  m.work_handler();

  delay(15); // IMPORTANT DELAY
}