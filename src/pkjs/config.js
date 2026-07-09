module.exports = [
  {
    type: 'heading',
    defaultValue: 'Klipper Monitor',
  },
  {
    type: 'section',
    items: [
      {
        type: 'heading',
        defaultValue: 'Printer Connection',
      },
      {
        type: 'input',
        messageKey: 'MoonrakerUrl',
        defaultValue: 'http://printer.lan',
        label: 'Moonraker URL',
        description:
          'The address of your printer, e.g. http://192.168.1.100 or http://hostname.local',
        attributes: {
          placeholder: 'http://',
          type: 'url',
        },
      },
    ],
  },
  {
    type: 'submit',
    defaultValue: 'Save Settings',
  },
];
