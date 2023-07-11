#include <manager/manager.h>

manager &m = manager::getInstance(); // invoke singleton instance

void setup()
{
  if (m.begin() != EXIT_SUCCESS)
    esp_restart(); // something went wrong

  delay(3000);
}

void loop()
{
  m.work_handler();
  delay(10); // IMPORTANT DELAY
}