import * as THREE from 'three';

// @ts-ignore
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls';
import { STLLoader } from 'three/examples/jsm/loaders/STLLoader';

import { WebSocketManager } from './websocket';
import TrackerSample from './tracker_sample.json';

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

class Tracker {
    private geometry: THREE.BufferGeometry | null = null;
    private material: THREE.MeshToonMaterial | null = null;
    private mesh: THREE.Mesh = new THREE.Mesh();

    constructor(url: string) {
        const loader = new STLLoader();
        loader.load(url, (geometry) => {
            this.geometry = geometry;
            this.material = new THREE.MeshToonMaterial({ color: 0x0000ff });
            this.mesh = new THREE.Mesh(this.geometry, this.material);
        });
    }

    public async getMesh() {
        return new Promise<THREE.Mesh>((resolve) => {
            const interval = setInterval(() => {
                if (this.mesh) {
                    clearInterval(interval);
                    resolve(this.mesh);
                }
            }, 100);
        });
    }

    public update(data: {
        returnValue: boolean,
        rotation: number[],
        translation: number[]
    }) {
        if (this.mesh) {
            // this.mesh.rotation.set(
            //     data.rotation[0],
            //     data.rotation[1],
            //     data.rotation[2]
            // );
            this.mesh.position.set(
                -data.translation[0],
                -data.translation[1],
                data.translation[2],
            );

            this.mesh.scale.set(0.001, 0.001, 0.001);
        }
    }
}

const scene = new THREE.Scene();
const camera = new THREE.PerspectiveCamera(75, window.innerWidth / window.innerHeight, 0.1, 1000);

const light = new THREE.DirectionalLight(0xffffff, 1);
light.position.set(0, 0, 1);
scene.add(light);

const axis = new THREE.AxesHelper(3);
scene.add(axis);


camera.position.z = -2;
camera.rotation.y = Math.PI;

const DirectionLines = [
    new DirectionLine(),
    new DirectionLine(),
    new DirectionLine(),
    new DirectionLine()
];

DirectionLines.forEach(line => scene.add(line.line));

const tracker = new Tracker("Tracker.stl");

tracker.getMesh().then((mesh) => {
    scene.add(mesh);
});

const renderer = new THREE.WebGLRenderer();
renderer.setSize(window.innerWidth, window.innerHeight);

const controls = new OrbitControls(camera, renderer.domElement);
controls.enableDamping = true;
controls.dampingFactor = 0.25;
controls.rotateSpeed = 0.35;
controls.zoomSpeed = 0.35;
controls.panSpeed = 0.35;

document.body.appendChild(renderer.domElement);

function mapRange(value: number, low1: number, high1: number, low2: number, high2: number) {
    return low2 + (high2 - low2) * (value - low1) / (high1 - low1);
}

function getVectorFromAngles(vertical: number, horizontal: number) {
    const x = Math.sin(vertical) * Math.cos(horizontal);
    const y = Math.sin(vertical) * Math.sin(horizontal);
    const z = Math.cos(vertical);

    return new THREE.Vector3(x, y, z).normalize().multiplyScalar(3);
}

function timeToAngle(time: number) {
    return mapRange(time, 96960, 542960, -60, 60);
}

websocketManager.registerCallback(async (data) => {
    // console.log(data);

    const vertical = data.vertical.map(timeToAngle);
    const horizontal = data.horizontal.map(timeToAngle);

    // console.log(vertical[0], horizontal[0]);

    for (let i = 0; i < 4; i++) {
        const vector = getVectorFromAngles(vertical[i] * Math.PI / 180, horizontal[i] * Math.PI / 180);
        DirectionLines[i].update(vector);
    }

    const fetched = await fetch("http://localhost:8081", {
        method: "POST",
        // mode: "no-cors",
        headers: {
            "Content-Type": "application/json",
            "Access-Control-Allow-Origin": "*"
        },
        body: JSON.stringify({
            horizontal,
            vertical,
        })
    });

    const response: {
        returnValue: boolean,
        rotation: number[],
        translation: number[]
    } = await fetched.json();

    if (response.returnValue) {
        tracker.update(response);
    }
});

function animate() {
    requestAnimationFrame(animate);

    controls.update();
    renderer.render(scene, camera);
}

animate();