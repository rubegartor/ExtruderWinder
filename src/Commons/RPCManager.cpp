#include "RPCManager.h"
#include <cstring>

#ifdef CORE_M4
#include "globals.h"
#endif

RPCManager::RPCManager() 
  : newSystemDataAvailable(false),
    newWebServerInfoAvailable(false),
    newMeasurementDataAvailable(false),
    sram4Base(nullptr) {

#ifdef CORE_M4
  lastSystemDataSendTime = 0;
  systemDataInterval = 100;  // 100ms por defecto
  autoSendSystemData = true; // Envío automático habilitado por defecto
#endif

  memset(&systemDataBuffer, 0, sizeof(SystemStatusMessage));
  memset(&webServerBuffer, 0, sizeof(WebServerMessage));
  memset(&measurementDataBuffer, 0, sizeof(MeasurementDataMessage));
}

void RPCManager::begin() {
  initSharedMemory();
}

void RPCManager::loop() {
  receiveMessages();

#ifdef CORE_M4
  if (autoSendSystemData) {
    unsigned long currentTime = millis();
    if (currentTime - lastSystemDataSendTime >= systemDataInterval) {
      updateAndSendSystemData();
      lastSystemDataSendTime = currentTime;
    }
  }
#endif
}

void RPCManager::initSharedMemory() {
  // Dirección base de SRAM4 compartida entre cores
  static constexpr uintptr_t SRAM4_BASE_ADDR = 0x38000000UL;
  sram4Base = reinterpret_cast<volatile uint8_t*>(SRAM4_BASE_ADDR);

  // Inicializar canales con offsets calculados automáticamente
  size_t offset = 0;
  
  // Canal 0: M4 → M7 (datos del sistema)
  initChannel(CHANNEL_SYSTEM_DATA, offset, sizeof(SystemStatusMessage));
  
  // Canal 1: M7 → M4 (información del servidor web)
  initChannel(CHANNEL_WEB_SERVER, offset, sizeof(WebServerMessage));
  
  // Canal 2: M7 → M4 (datos de medición)
  initChannel(CHANNEL_MEASUREMENT, offset, sizeof(MeasurementDataMessage));
}

void RPCManager::initChannel(ChannelType type, size_t& offset, size_t dataSize) {
  // Control del canal
  channels[type].control = reinterpret_cast<ChannelControl*>(const_cast<uint8_t*>(sram4Base + offset));
  offset += sizeof(ChannelControl);
  offset = alignTo8Bytes(offset);
  
  // Datos del canal
  channels[type].data = sram4Base + offset;
  channels[type].dataSize = dataSize;
  offset += dataSize;
  offset = alignTo8Bytes(offset);

  // Inicializar control según el core y tipo de canal
  bool shouldInit = false;
#ifdef CORE_M4
  shouldInit = (type == CHANNEL_SYSTEM_DATA);
#elif defined(CORE_M7)
  shouldInit = (type == CHANNEL_WEB_SERVER || type == CHANNEL_MEASUREMENT);
#endif

  if (shouldInit && channels[type].control) {
    channels[type].control->dataReady = false;
    channels[type].control->messageSize = dataSize;
  }
}

void RPCManager::receiveMessages() {
  // Procesar todos los canales de recepción según el core
#ifdef CORE_M4
  // M4 recibe datos del servidor web y medición
  if (receiveData(CHANNEL_WEB_SERVER, &webServerBuffer, sizeof(WebServerMessage))) {
    newWebServerInfoAvailable = true;
  }
  if (receiveData(CHANNEL_MEASUREMENT, &measurementDataBuffer, sizeof(MeasurementDataMessage))) {
    newMeasurementDataAvailable = true;
  }
#elif defined(CORE_M7)
  // M7 recibe datos del sistema
  if (receiveData(CHANNEL_SYSTEM_DATA, &systemDataBuffer, sizeof(SystemStatusMessage))) {
    newSystemDataAvailable = true;
  }
#endif
}

bool RPCManager::sendData(ChannelType channel, const void* data, size_t size) {
  if (channel >= CHANNEL_COUNT || !channels[channel].control || !data) {
    return false;
  }

  // Verificar que el puntero de datos sea válido
  if (!channels[channel].data) {
    return false;
  }

  // Para datos de medición, permitir sobrescribir datos no leídos
  bool allowOverwrite = (channel == CHANNEL_MEASUREMENT);
  
  if (!allowOverwrite && channels[channel].control->dataReady) {
    return false;  // No sobrescribir datos importantes no leídos
  }

  if (size > channels[channel].dataSize || size == 0) {
    return false;  // Datos demasiado grandes o vacíos
  }

  // Copia segura con verificación de límites
  volatile uint8_t* dest = const_cast<volatile uint8_t*>(channels[channel].data);
  const uint8_t* src = static_cast<const uint8_t*>(data);
  
  for (size_t i = 0; i < size; i++) {
    dest[i] = src[i];
  }
  
  __sync_synchronize();
  channels[channel].control->messageSize = size;
  channels[channel].control->dataReady = true;
  
  return true;
}

bool RPCManager::receiveData(ChannelType channel, void* buffer, size_t bufferSize) {
  if (channel >= CHANNEL_COUNT || !channels[channel].control || !buffer || bufferSize == 0) {
    return false;
  }

  // Verificar que el puntero de datos sea válido
  if (!channels[channel].data) {
    return false;
  }

  if (!channels[channel].control->dataReady) {
    return false;  // No hay datos nuevos
  }

  uint32_t messageSize = channels[channel].control->messageSize;
  if (messageSize > bufferSize || messageSize == 0) {
    return false;  // Buffer demasiado pequeño o mensaje inválido
  }

  // Copia segura con verificación de límites
  uint8_t* dest = static_cast<uint8_t*>(buffer);
  const volatile uint8_t* src = channels[channel].data;
  
  for (size_t i = 0; i < messageSize; i++) {
    dest[i] = src[i];
  }
  
  channels[channel].control->dataReady = false;
  __sync_synchronize();
  
  return true;
}

// ===== MÉTODOS PARA CORE M4 =====
#ifdef CORE_M4
void RPCManager::updateAndSendSystemData() {
  updateSystemData();
  sendSystemData();
}

void RPCManager::updateSystemData() {
  systemDataBuffer.currentDiameter = diameter;
  systemDataBuffer.plasticTypeIndex = selectedPlasticTypeIndex;

  systemDataBuffer.pullerSpeed = puller.speed;
  systemDataBuffer.pullerRevolutions = puller.getRevolutionCount();
  systemDataBuffer.spoolerSpeed = spooler.speed;
  systemDataBuffer.spoolerRevolutions = spooler.getRevolutionCount();
  systemDataBuffer.alignerPosition = aligner.currentPosition();
  systemDataBuffer.alignerState = static_cast<uint8_t>(aligner.getCurrentState());
  systemDataBuffer.alignerDirection = static_cast<uint8_t>(aligner.getCurrentDirection());

  systemDataBuffer.tensionerDistance = tensioner.distance;

  systemDataBuffer.isSpoolCalibrated = aligner.isSpoolCalibrated();
  systemDataBuffer.wifiEnabled = wifiEnabled;
  systemDataBuffer.resetMeasurementsFlag = false; // Se establecerá en true cuando sea necesario

  systemDataBuffer.extrudedLength = getExtrudedFilamentLength();
  systemDataBuffer.extrudedWeight = getExtrudedFilamentWeight();

  systemDataBuffer.pullerMinOutput = puller.minOutput;
  systemDataBuffer.pullerMaxOutput = puller.maxOutput;

  systemDataBuffer.alignerStartPos = aligner.startPos;
  systemDataBuffer.alignerEndPos = aligner.endPos;
  systemDataBuffer.alignerStartExtension = aligner.getStartExtension();
  systemDataBuffer.alignerEndExtension = aligner.getEndExtension();
  systemDataBuffer.alignerMaxPosition = MAX_ALIGNER_POSITION;

  systemDataBuffer.timestamp = millis();
}

void RPCManager::sendSystemData() {
  sendData(CHANNEL_SYSTEM_DATA, &systemDataBuffer, sizeof(SystemStatusMessage));
}

void RPCManager::requestMeasurementReset() {
  systemDataBuffer.resetMeasurementsFlag = true;
  sendSystemData();
  systemDataBuffer.resetMeasurementsFlag = false; // Resetear el flag después de enviar
}
#endif

// ===== MÉTODOS PARA CORE M7 =====
#ifdef CORE_M7
void RPCManager::sendWiFiInfo(const char* ip) {
  WebServerMessage config = {};

  if (ip != nullptr) {
    strncpy(config.wifiIP, ip, sizeof(config.wifiIP) - 1);
    config.wifiIP[sizeof(config.wifiIP) - 1] = '\0';
  }
  
  config.timestamp = millis();
  
  sendWebServerInfo(config);
}

void RPCManager::sendWebServerInfo(const WebServerMessage& config) {
  sendData(CHANNEL_WEB_SERVER, &config, sizeof(WebServerMessage));
}

void RPCManager::sendMeasurementData(float lastRead, float minRead, float maxRead) {
  MeasurementDataMessage measurement = {};
  measurement.lastRead = lastRead;
  measurement.minRead = minRead;
  measurement.maxRead = maxRead;
  measurement.timestamp = millis();
  
  sendMeasurementData(measurement);
}

void RPCManager::sendMeasurementData(const MeasurementDataMessage& measurement) {
  sendData(CHANNEL_MEASUREMENT, &measurement, sizeof(MeasurementDataMessage));
}
#endif