# Generate short warning-tone WAVs for the roadbook plugin (played via PlaySound).
import wave, struct, math, os
DST = r'D:\BikesRoadbook\data\audio'
os.makedirs(DST, exist_ok=True)
SR = 22050

def env(i, n, atk=0.008, rel=0.03):
    a = max(1, int(atk * SR)); r = max(1, int(rel * SR))
    if i < a: return i / a
    if i > n - r: return max(0.0, (n - i) / r)
    return 1.0

def synth(segments, vol=0.38):
    data = []
    for freq, dur in segments:
        n = int(dur * SR)
        for i in range(n):
            s = (math.sin(2 * math.pi * freq * i / SR) if freq > 0 else 0.0) * env(i, n) * vol
            data.append(s)
    return data

def write(name, data):
    w = wave.open(os.path.join(DST, name), 'w')
    w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR)
    w.writeframes(b''.join(struct.pack('<h', int(max(-1.0, min(1.0, s)) * 32767)) for s in data))
    w.close()

write('rb_chime.wav',    synth([(988, 0.07), (1319, 0.13)]))                                  # box approaching
write('rb_danger.wav',   synth([(1568, 0.06), (0, 0.05), (1568, 0.06), (0, 0.05), (1568, 0.10)], vol=0.42))  # danger box ahead
write('rb_offroute.wav', synth([(720, 0.09), (1040, 0.09), (720, 0.09), (1040, 0.11)], vol=0.42))            # off route
print('wrote', os.listdir(DST))
