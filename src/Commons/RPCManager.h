#pragma once

#include <stdint.h>
#include <Arduino.h>
#include <string.h>

// Definitions for IntelliSense when core-specific macros are not available
#ifdef __INTELLISENSE__
  #ifndef CORE_M4
    #define CORE_M4
  #endif
  #ifndef CORE_M7
    #define CORE_M7
  #endif
#endif

/**
 * Mensaje que contiene todos los datos del sistema del Core M4
 * Este mensaje se envía desde M4 hacia M7 con información del estado del sistema
 */
struct SystemStatusMessage {
  // Configuración del filamento
  float currentDiameter;
  uint8_t plasticTypeIndex;

  // Estado de los motores
  uint16_t pullerSpeed;
  int32_t pullerRevolutions;
  uint16_t spoolerSpeed;
  int32_t spoolerRevolutions;
  int32_t alignerPosition;
  uint8_t alignerState;      // AlignerState como uint8_t
  uint8_t alignerDirection;  // AlignerDirection como uint8_t

  // Estado del tensioner
  uint16_t tensionerDistance;

  // Estados del sistema
  bool isSpoolCalibrated;
  bool wifiEnabled;
  bool resetMeasurementsFlag; // Flag para indicar reset de mediciones

  // Estadísticas de extrusión
  float extrudedLength;      // en metros
  float extrudedWeight;      // en gramos

  // Configuraciones de los motores
  int32_t pullerMinOutput;
  int32_t pullerMaxOutput;
  int32_t alignerStartPos;
  int32_t alignerEndPos;
  int32_t alignerStartExtension;
  int32_t alignerEndExtension;
  int32_t alignerMaxPosition;

  unsigned long timestamp;
} __attribute__((packed));

/**
 * Mensaje de datos de medición que se envía desde M7 hacia M4
 * Contiene los valores de medición actuales
 */
struct MeasurementDataMessage {
  float lastRead;
  float minRead;
  float maxRead;
  unsigned long timestamp;
} __attribute__((packed));

/**
 * Mensaje del servidor web que se envía desde M7 hacia M4
 * Contiene información del WiFi
 */
struct WebServerMessage {
  // Información del WiFi
  char wifiIP[16];           // IP del servidor web (formato "192.168.1.100")
  
  // Timestamp para validar la frescura del mensaje
  unsigned long timestamp;
} __attribute__((packed));

/**
 * Gestiona la comunicación bidireccional entre cores M4 y M7
 * 
 * Flujo de comunicación:
 * - M4 → M7: SystemStatusMessage (datos del sistema, sensores, motores)
 * - M7 → M4: WebServerMessage (IP WiFi)
 * - M7 → M4: MeasurementDataMessage (datos de medición)
 * 
 * Uso básico:
 * 
 * En Core M4:
 *   rpcManager.begin();
 *   rpcManager.updateAndSendSystemData();  // Actualiza y envía datos del sistema
 *   if (rpcManager.hasNewWebServerInfo()) {
 *     auto config = rpcManager.getWebServerInfo();
 *     // Usar config.wifiIP
 *   }
 *   if (rpcManager.hasNewMeasurementData()) {
 *     auto measurement = rpcManager.getMeasurementData();
 *     // Usar measurement.lastRead, minRead, maxRead
 *   }
 * 
 * En Core M7:
 *   rpcManager.begin();
 *   if (rpcManager.hasNewSystemData()) {
 *     auto data = rpcManager.getSystemData();
 *     // Usar data para UI, web server, etc.
 *   }
 *   rpcManager.sendWiFiInfo(ip);  // Enviar IP WiFi
 *   rpcManager.sendMeasurementData(lastRead, minRead, maxRead);  // Enviar datos de medición
 */
class RPCManager {
public:
  RPCManager();
  
  /**
   * Inicializa la comunicación entre cores
   * Debe llamarse en setup() de ambos cores
   */
  void begin();
  
  /**
   * Maneja la comunicación automática
   * Debe llamarse en loop() de ambos cores
   */
  void loop();

  // ===== MÉTODOS PARA CORE M4 =====
#ifdef CORE_M4
  /**
   * Actualiza los datos del sistema con los valores actuales y los envía al M7
   * Combina updateSystemData() y sendSystemData() en una sola llamada
   */
  void updateAndSendSystemData();
  
  /**
   * Actualiza solo los datos del sistema sin enviarlos
   * Útil si quieres controlar cuándo se envían los datos
   */
  void updateSystemData();
  
  /**
   * Envía los datos del sistema al M7
   * Los datos deben haberse actualizado previamente con updateSystemData()
   */
  void sendSystemData();
  
  /**
   * Configura el intervalo de envío automático de datos
   * @param ms Intervalo en milisegundos (por defecto 100ms)
   */
  void setSystemDataInterval(unsigned long ms) { systemDataInterval = ms; }
  
  /**
   * Habilita/deshabilita el envío automático de datos en loop()
   * @param enabled true para envío automático, false para envío manual
   */
  void setAutoSendSystemData(bool enabled) { autoSendSystemData = enabled; }
  
  /**
   * Establece el flag para resetear las mediciones en el Core M7
   */
  void requestMeasurementReset();
#endif

  // ===== MÉTODOS PARA CORE M7 =====

  // ===== MÉTODOS PARA CORE M7 =====
#if defined(CORE_M7) || defined(__INTELLISENSE__)
  /**
   * Envía configuración WiFi al M4
   * @param ip Dirección IP del servidor web (ej: "192.168.1.100")
   */
  void sendWiFiInfo(const char* ip);
  
  /**
   * Envía información del servidor web al M4
   * @param config Estructura con la información del servidor web
   */
  void sendWebServerInfo(const WebServerMessage& config);
  
  /**
   * Envía datos de medición al M4
   * @param lastRead Última lectura
   * @param minRead Lectura mínima
   * @param maxRead Lectura máxima
   */
  void sendMeasurementData(float lastRead, float minRead, float maxRead);
  
  /**
   * Envía datos de medición al M4
   * @param measurement Estructura con los datos de medición
   */
  void sendMeasurementData(const MeasurementDataMessage& measurement);
#endif

  // ===== MÉTODOS COMUNES PARA AMBOS CORES =====
  
  /**
   * Verifica si hay nuevos datos del sistema disponibles (desde M4)
   * @return true si hay datos nuevos sin procesar
   */
  bool hasNewSystemData() const { return newSystemDataAvailable; }
  
  /**
   * Obtiene los datos del sistema más recientes
   * @return Referencia constante a los datos del sistema
   */
  const SystemStatusMessage& getSystemData() const { return systemDataBuffer; }
  
  /**
   * Marca los datos del sistema como procesados
   * Debe llamarse después de procesar los datos para evitar procesarlos múltiples veces
   */
  void markSystemDataProcessed() { newSystemDataAvailable = false; }
  
  /**
   * Verifica si hay nueva información del servidor web disponible (desde M7)
   * @return true si hay información nueva sin procesar
   */
  bool hasNewWebServerInfo() const { return newWebServerInfoAvailable; }
  
  /**
   * Obtiene la información del servidor web más reciente
   * @return Referencia constante a la información del servidor web
   */
  const WebServerMessage& getWebServerInfo() const { return webServerBuffer; }
  
  /**
   * Marca la información del servidor web como procesada
   * Debe llamarse después de procesar la información del servidor web
   */
  void markWebServerInfoProcessed() { newWebServerInfoAvailable = false; }
  
  /**
   * Verifica si hay nuevos datos de medición disponibles (desde M7)
   * @return true si hay datos nuevos sin procesar
   */
  bool hasNewMeasurementData() const { return newMeasurementDataAvailable; }
  
  /**
   * Obtiene los datos de medición más recientes
   * @return Referencia constante a los datos de medición
   */
  const MeasurementDataMessage& getMeasurementData() const { return measurementDataBuffer; }
  
  /**
   * Marca los datos de medición como procesados
   * Debe llamarse después de procesar los datos de medición
   */
  void markMeasurementDataProcessed() { newMeasurementDataAvailable = false; }

private:
  // Enumeración para tipos de canal
  enum ChannelType {
    CHANNEL_SYSTEM_DATA = 0,    // M4 → M7
    CHANNEL_WEB_SERVER = 1,     // M7 → M4
    CHANNEL_MEASUREMENT = 2,    // M7 → M4
    CHANNEL_COUNT = 3
  };

  // Estructura para sincronización en memoria compartida
  struct __attribute__((aligned(8))) ChannelControl { 
    volatile bool dataReady;
    volatile uint32_t messageSize;
  };

  // Estructura simplificada de canal
  struct Channel {
    ChannelControl* control;
    volatile uint8_t* data;
    size_t dataSize;
  };

  // Buffers para los mensajes
  SystemStatusMessage systemDataBuffer;
  WebServerMessage webServerBuffer;
  MeasurementDataMessage measurementDataBuffer;
  
  // Flags para datos nuevos
  bool newSystemDataAvailable;
  bool newWebServerInfoAvailable;
  bool newMeasurementDataAvailable;

#ifdef CORE_M4
  // Variables para el envío automático desde M4
  unsigned long lastSystemDataSendTime;
  unsigned long systemDataInterval;
  bool autoSendSystemData;
#endif

  // Array de canales unificado
  Channel channels[CHANNEL_COUNT];
  
  // Puntero base a memoria compartida SRAM4
  volatile uint8_t* sram4Base;

  /**
   * Inicializa la memoria compartida SRAM4
   */
  void initSharedMemory();
  
  /**
   * Inicializa un canal específico
   */
  void initChannel(ChannelType type, size_t& offset, size_t dataSize);
  
  /**
   * Alinea un offset a 8 bytes
   */
  constexpr size_t alignTo8Bytes(size_t offset) const {
    return (offset + 7) & ~size_t(7);
  }
  
  /**
   * Recibe todos los mensajes disponibles
   */
  void receiveMessages();
  
  /**
   * Envía datos a través del canal especificado
   */
  bool sendData(ChannelType channel, const void* data, size_t size);
  
  /**
   * Recibe datos desde el canal especificado
   */
  bool receiveData(ChannelType channel, void* buffer, size_t bufferSize);
  
  /**
   * Obtiene el buffer de destino para un tipo de canal
   */
  void* getChannelBuffer(ChannelType channel);
  
  /**
   * Obtiene el flag de datos nuevos para un tipo de canal
   */
  bool* getNewDataFlag(ChannelType channel);
};
