import viveka

def start():
    print("Starting...")

def end():
    print("Ending...")

ctx = viveka.Profiler()

viveka.Event(ctx, 'start')
start()

viveka.Event(ctx, 'end')
end()

