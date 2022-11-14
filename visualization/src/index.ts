import * as THREE from 'three';
import { WebSocketManager } from './websocket'; 

const websocketManager = new WebSocketManager();

class DirectionLine {
    private geometry: THREE.BufferGeometry;
    private material: THREE.LineBasicMaterial;
    public line: THREE.Line;

    private origin = new THREE.Vector3(0, 0, 0);

    constructor() {
        this.geometry = new THREE.BufferGeometry();
        this.material = new THREE.LineBasicMaterial({ color: 0x00ff00 });
        this.line = new THREE.Line(this.geometry, this.material);
    }

    public update(data: THREE.Vector3) {
        this.geometry.setFromPoints([
            this.origin,
            data
        ]);
    }
}

const scene = new THREE.Scene();
const camera = new THREE.PerspectiveCamera(75, window.innerWidth / window.innerHeight, 0.1, 1000);

camera.position.z = 2;

const DirectionLines = [
    new DirectionLine(),
    new DirectionLine(),
    new DirectionLine(),
    new DirectionLine()
];

DirectionLines.forEach(line => scene.add(line.line));

const renderer = new THREE.WebGLRenderer();
renderer.setSize(window.innerWidth, window.innerHeight);

document.body.appendChild(renderer.domElement);

function map_range(value: number, low1: number, high1: number, low2: number, high2: number) {
    return low2 + (high2 - low2) * (value - low1) / (high1 - low1);
}

websocketManager.registerCallback((data) => {
    const horizontal = data.horizontal.map((value) => map_range(value, 96960, 542960, -60, 60));
    const vertical = data.vertical.map((value) => map_range(value, 96960, 542960, -60, 60));

    function getVectorFromAngles(horizontal: number, vertical: number) {
        const x = Math.cos(vertical) * Math.sin(horizontal);
        const y = Math.sin(vertical);
        const z = Math.cos(vertical) * Math.cos(horizontal);

        return new THREE.Vector3(x, y, z);
    }

    for (let i = 0; i < 4; i++) {
        DirectionLines[i].update(getVectorFromAngles(horizontal[i] * Math.PI / 180, vertical[i] * Math.PI / 180));
    }
});

function animate() {
    requestAnimationFrame(animate);
    renderer.render(scene, camera);
}

animate();