export interface TrackerData {
    horizontal: [number, number, number, number];
    vertical: [number, number, number, number];
}

export class WebSocketManager
{
    private socket: WebSocket;
    private callback: ((data: TrackerData) => void) | null = null;

    private data: TrackerData = {
        horizontal: [0, 0, 0, 0],
        vertical: [0, 0, 0, 0]
    };

    constructor()
    {
        this.socket = new WebSocket('ws://192.168.50.30/ws');
        this.socket.onmessage = this.onMessage.bind(this);

        this.socket.onopen = () => {
            this.socket.send('Hello Server!');
        };
    }

    public registerCallback(callback: (data: TrackerData) => void)
    {
        this.callback = callback;
    }

    public async onMessage(event: MessageEvent)
    {   
        const data = event.data as Blob;
        const value = new Uint8Array(await data.arrayBuffer());

        // first byte is the axis
        const axis = value[0];

        // last 16 bytes are uint32 values
        const values = new Uint32Array(value.buffer.slice(1));

        if (axis) {
            this.data.horizontal = [values[0], values[1], values[2], values[3]];
        }
        else {
            this.data.vertical = [values[0], values[1], values[2], values[3]];
        }

        this.socket.send('Got it!');

        if (this.callback) {
            this.callback(this.data);
        }
    }       
}