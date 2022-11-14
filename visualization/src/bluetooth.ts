const SERVICE_UUID = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
const CHARACTERISTIC_UUID = "6e400003-b5a3-f393-e0a9-e50e24dcca9e";

export interface TrackerData {
    horizontal: [number, number, number, number];
    vertical: [number, number, number, number];
}

export class BluetoothManager {
    device: BluetoothDevice | null = null;
    callback: ((data: TrackerData) => void) | null = null;

    data: TrackerData = {
        horizontal: [0, 0, 0, 0],
        vertical: [0, 0, 0, 0]
    };

    subscribe() {
        navigator.bluetooth.requestDevice({
            optionalServices: [SERVICE_UUID],
            acceptAllDevices: true
        }).then(device => {
            console.log('Got device: ' + device.name);
            this.device = device;
            return device.gatt!.connect();
        }).then(server => {
            console.log('Got server');
            return server.getPrimaryService(SERVICE_UUID);
        }).then(service => {
            console.log('Got service');
            return service.getCharacteristic(CHARACTERISTIC_UUID);
        }).then(characteristic => {
            characteristic.addEventListener('characteristicvaluechanged', this.onValueChanged);
            return characteristic.startNotifications();
        }).catch(error => {
            console.log('Argh! ' + error);
        });
    }

    private onValueChanged(event: Event) {
        console.log('Got value');

        const characteristic = event.target as BluetoothRemoteGATTCharacteristic;
        const value = new Uint8Array(characteristic.value!.buffer);

        // first byte is the axis
        const axis = value[0];

        // last 16 bytes are uint32 values
        const values = new Uint32Array(value.buffer.slice(1));

        if (!axis) {
            this.data.horizontal = [values[0], values[1], values[2], values[3]];
        }
        else {
            this.data.vertical = [values[0], values[1], values[2], values[3]];
        }

        if (this.callback) {
            this.callback(this.data);
        }
    }

    public registerCallback(callback: (data: TrackerData) => void) {
        this.callback = callback;
    }
}