import sys
import re
import matplotlib.pyplot as plt
from matplotlib.lines import Line2D

# ==========================================
# 1. LETTURA DEL LOG DA FILE
# ==========================================
filename = "analysis/log.out"
if len(sys.argv) > 1:
    filename = sys.argv[1]

try:
    with open(filename, 'r', encoding='utf-8') as file:
        log_text = file.read()
except FileNotFoundError:
    print(f"Errore: Impossibile trovare il file '{filename}'.")
    sys.exit(1)

# ==========================================
# 2. LOGICA DI PARSING MATEMATICA
# ==========================================
tick_pattern = re.compile(r"^\[(\d+(?:\.\d+)?)\]\s+(.*)")
preempt_pattern = re.compile(r"PREEMPTING: Task ([\w\d]+) -> ([\w\d]+)")

intervals = []
finish_events = [] 
current_task = None
current_start = 0.0
last_tick = 0.0
time_offset = None

for line in log_text.strip().split('\n'):
    match = tick_pattern.search(line)
    if not match: continue
        
    raw_tick = float(match.group(1))
    msg = match.group(2)
    
    if "Avvio del sistema" in msg: continue
    if time_offset is None: time_offset = raw_tick
        
    tick = raw_tick - time_offset
    last_tick = tick

    if preempt_match := preempt_pattern.search(msg):
        task_activated = preempt_match.group(2)
        if current_task is not None and current_start < tick:
            intervals.append({'task': current_task, 'start': current_start, 'end': tick})
            
        current_task = task_activated
        current_start = tick
        
    elif "FINISHED" in msg:
        task_name_match = re.search(r"\[([\w_]+)\]\s+FINISHED", msg)
        if task_name_match:
            name = task_name_match.group(1)
            t_id = "0" if "Flight" in name else "1" if "Telem" in name else "2" if "Nav" in name else None
            if t_id:
                finish_events.append({'task': t_id, 'tick': tick})

if current_task is not None and current_start <= last_tick:
    intervals.append({'task': current_task, 'start': current_start, 'end': last_tick})

# ==========================================
# 3. DISEGNO DEL GRAFICO CON VISUAL SNAPPING
# ==========================================
task_map = {
    "0": {"name": "Task 0 (Flight)", "color": "#e74c3c", "y": 3},
    "1": {"name": "Task 1 (Telem)", "color": "#f1c40f", "y": 2},
    "2": {"name": "Task 2 (Nav)", "color": "#3498db", "y": 1},
    "IDLE": {"name": "CPU IDLE", "color": "#95a5a6", "y": 0}
}

fig, ax = plt.subplots(figsize=(12, 6))
MIN_WIDTH = 0.3  # Larghezza minima per garantire la visibilità dei micro-task

# 1. Disegna le barre e calcola il bordo visivo reale
for interval in intervals:
    t_id = interval['task']
    if t_id not in task_map: continue
        
    y_pos = task_map[t_id]['y']
    start = interval['start']
    width = interval['end'] - interval['start']
    
    # Calcola e salva la larghezza visiva espansa
    visual_width = max(width, MIN_WIDTH)
    interval['visual_end'] = start + visual_width 
    
    ax.barh(y_pos, visual_width, left=start, height=0.6, align='center', 
            color=task_map[t_id]['color'], edgecolor='black', linewidth=0.5, zorder=2)

# 2. Disegna le X agganciandole al bordo destro visivo calcolato
for fe in finish_events:
    t_id = fe['task']
    f_tick = fe['tick']
    
    target_visual_end = f_tick
    # Trova l'intervallo che era in esecuzione in quel millisecondo
    for interval in intervals:
        if interval['task'] == t_id and interval['start'] <= f_tick + 0.001:
            target_visual_end = interval['visual_end']
            
    y_pos = task_map[t_id]['y']
    
    # Piazza la X esattamente sul bordo destro della barra stampata a schermo
    ax.plot(target_visual_end, y_pos, marker='x', color='black', markersize=10, markeredgewidth=2.5, zorder=3)

# Formattazione assi
ax.set_yticks([0, 1, 2, 3])
ax.set_yticklabels([task_map["IDLE"]["name"], task_map["2"]["name"], task_map["1"]["name"], task_map["0"]["name"]])
ax.set_xlabel('Tempo Reale (Millisecondi decimali)', fontsize=12, fontweight='bold')
ax.set_title('RTOS Real-Time Timeline (Microsecond Precision)', fontsize=16, fontweight='bold')

ax.xaxis.grid(True, linestyle='--', alpha=0.7, zorder=1)
ax.set_xlim(0, last_tick + 1)

legend_elements = [Line2D([0], [0], marker='x', color='w', label='Task Terminato (Fine Ciclo)',
                          markerfacecolor='black', markeredgecolor='black', markersize=10, markeredgewidth=2.5)]
ax.legend(handles=legend_elements, loc='upper right')

plt.tight_layout()
plt.show()