#pragma once

#include <Arduino.h>

class TimedComponent {
  protected:
    unsigned long lastExecutionTime = 0;
    unsigned long intervalMs = 0;

  public:
    // Constructor que permite establecer el intervalo en milisegundos
    TimedComponent(unsigned long interval = 0) : intervalMs(interval) {}
    
    // Método para cambiar el intervalo desde el main
    void setInterval(unsigned long interval) {
      intervalMs = interval;
    }
    
    // Método que verifica si es momento de ejecutar
    bool shouldExecute() {
      unsigned long currentTime = millis();
      if (currentTime - lastExecutionTime >= intervalMs) {
        lastExecutionTime = currentTime;
        return true;
      }
      return false;
    }
    
    // Método virtual que deben implementar las clases derivadas
    virtual void execute() = 0;
    
    // Método loop virtual que controla el timing - puede ser sobrescrito
    virtual void loop() {
      if (shouldExecute()) {
        execute();
      }
    }
    
    // Getter para obtener el intervalo actual
    unsigned long getInterval() const { return intervalMs; }
};
