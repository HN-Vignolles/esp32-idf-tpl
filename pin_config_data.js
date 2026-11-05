/*
 * Module and development-board hardware profiles for ESP32 PIN STUDIO.
 *
 * This is a classic script, rather than a fetched JSON file, so the configurator
 * can also be opened directly from disk with file://.
 *
 * Layout arrays are in screen order. `number` is the physical module pad or
 * header pin number, never a GPIO number. Numbers in `pin` identify GPIOs;
 * strings identify supply, ground, or enable pads. Development-board rows also
 * identify their header with `connector`.
 *
 * Modules describe the exposed pads and memory reservations. Board profiles
 * refer to a module by `module` and add header routing and fixed PCB connections.
 * Chip peripheral capabilities and IO MUX mappings belong in the chip data,
 * independently of these physical module and board definitions.
 *
 * Local references:
 *   misc/esp32-s3_datasheet_en.pdf, sections 2.3.4 and 2.6.
 *   misc/SCH_ESP32-S3-DevKitC-1_V1.1_20220413.pdf.
 * Online references are included in ESP32_PIN_DATA.sources below.
 */
(function () {
  'use strict';

  const moduleLayout = {
    left: ['GND', '3V3', 'EN', 4, 5, 6, 7, 15, 16, 17, 18, 8, 19, 20]
      .map((pin, index) => ({pin, number: index + 1})),
    bottom: [3, 46, 9, 10, 11, 12, 13, 14, 21, 47, 48, 45]
      .map((pin, index) => ({pin, number: index + 15})),
    right: ['GND', 1, 2, 43, 44, 42, 41, 40, 39, 38, 37, 36, 35, 0]
      .map((pin, index) => ({pin, number: 40 - index}))
  };

  const devkitLayout = {
    left: ['3V3', '3V3', 'EN', 4, 5, 6, 7, 15, 16, 17, 18, 8, 3, 46, 9, 10, 11, 12, 13, 14, '5V', 'GND']
      .map((pin, index) => ({pin, number: index + 1, connector: 'J1'})),
    right: ['GND', 43, 44, 1, 2, 42, 41, 40, 39, 38, 37, 36, 35, 0, 45, 48, 47, 21, 20, 19, 'GND', 'GND']
      .map((pin, index) => ({pin, number: index + 1, connector: 'J3'})),
    bottom: []
  };

  const unexposed = [
    {
      pins: [26, 27, 28, 29, 30, 31, 32],
      reason: 'The SPI0/1 flash/PSRAM bus is internal to the WROOM-1 module and is not exposed on its pads.'
    },
    {
      pins: [33, 34],
      reason: 'These chip GPIOs are not exposed on the WROOM-1 module. Their availability inside the chip also depends on the memory interface.'
    }
  ];

  const modulePresentation = {
    silkLines: ['ESP32-S3 · RF MODULE', 'TOP VIEW'],
    moduleName: 'ESP32-S3-WROOM',
    moduleBrand: 'ESPRESSIF',
    revision: null,
    footer: 'MODULE · 40 EDGE PADS',
    diagramLabel: 'Top view • WROOM-1 40-pad footprint',
    usbConnector: false,
    componentRows: []
  };

  function makeModule(name, short, flash, psram, octal) {
    return {
      name,
      short,
      kind: 'module',
      flash,
      psram,
      octal,
      default: {},
      layout: moduleLayout,
      connections: {},
      unexposed,
      reserves: octal ? [{
        pins: [35, 36, 37],
        reason: 'These module pads are connected to Octal PSRAM and cannot be assigned to other functions.'
      }] : [],
      presentation: {
        ...modulePresentation,
        subnote: octal
          ? 'Memory variant: GPIO35, GPIO36 and GPIO37 are wired to Octal PSRAM and cannot be assigned.'
          : 'Pins shown are external pads/headers. Flash bus GPIO26–32 is internal to the WROOM module and not brought out.'
      }
    };
  }

  const modules = {
    'wroom-n8': makeModule('ESP32-S3-WROOM-1-N8', 'WROOM-1-N8', '8 MB', 'None', false),
    'wroom-n16r8': makeModule('ESP32-S3-WROOM-1-N16R8', 'WROOM-1-N16R8', '16 MB', '8 MB Octal', true)
  };

  // A connection persists physically even when its configured signal changes.
  // `signal`, when present, supplies that connection's initial assignment.
  const devkitConnections = {
    0: {label: 'BOOT', description: 'BOOT pushbutton'},
    19: {label: 'USB D−', description: 'Native USB D− connector', signal: 'USB_D-'},
    20: {label: 'USB D+', description: 'Native USB D+ connector', signal: 'USB_D+'},
    38: {label: 'RGB LED', description: 'Onboard SK68XX RGB LED', signal: 'RGB_LED'},
    43: {label: 'UART TX', description: 'UART0 TX to USB–UART bridge RX', signal: 'UART0_TX'},
    44: {label: 'UART RX', description: 'UART0 RX from USB–UART bridge TX', signal: 'UART0_RX'}
  };

  const devkitPresentation = {
    silkLines: ['ESP32-S3 · DEVKITC-1', 'REV 1.1 · ESPRESSIF'],
    moduleName: 'ESP32-S3-WROOM-1',
    moduleBrand: 'ESPRESSIF',
    revision: '1.1',
    footer: 'J1 · 22 PINS   |   J3 · 22 PINS',
    diagramLabel: 'Top view • J1 / J3 headers • board revision 1.1',
    usbConnector: true,
    componentRows: [
      [
        {label: 'BOOT', tone: 'orange'},
        {label: 'RGB LED', tone: ''},
        {label: 'RESET', tone: 'orange'}
      ],
      [
        {label: 'USB / UART', tone: 'dark'},
        {label: 'NATIVE USB', tone: 'dark'}
      ]
    ]
  };

  function makeDevkit(moduleId, variant) {
    const module = modules[moduleId];
    return {
      ...module,
      module: moduleId,
      name: `ESP32-S3-DevKitC-1 v1.1 ${variant}`,
      short: `DevKitC-1 ${variant}`,
      kind: 'devkit',
      layout: devkitLayout,
      connections: devkitConnections,
      default: Object.fromEntries(Object.entries(devkitConnections)
        .filter(([, connection]) => connection.signal)
        .map(([pin, connection]) => [pin, connection.signal])),
      presentation: {...devkitPresentation, subnote: module.presentation.subnote}
    };
  }

  // Module profiles are also selectable independently of a development board.
  const boards = {
    'wroom-n8': {...modules['wroom-n8'], module: 'wroom-n8'},
    'wroom-n16r8': {...modules['wroom-n16r8'], module: 'wroom-n16r8'},
    'devkit-n8': makeDevkit('wroom-n8', 'N8'),
    'devkit-n16r8': makeDevkit('wroom-n16r8', 'N16R8')
  };

  window.ESP32_PIN_DATA = {
    modules,
    boards,
    sources: [
      {
        title: 'ESP32-S3 Series Datasheet',
        path: 'misc/esp32-s3_datasheet_en.pdf',
        url: 'https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf'
      },
      {
        title: 'ESP32-S3-DevKitC-1 v1.1 schematic',
        path: 'misc/SCH_ESP32-S3-DevKitC-1_V1.1_20220413.pdf'
      },
      {
        title: 'ESP32-S3-DevKitC-1 v1.1 user guide',
        url: 'https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/user_guide_v1.1.html'
      }
    ]
  };
}());
