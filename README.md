# ets2la-plugin
Please see the [original repository](https://github.com/dariowouters/ts-extra-utilities) by dariowouters!

# What does this do?
- [x] Expose a virtual memory file to `Local\ETS2LAPluginStatus` to get the current plugin status. (Use this if you want to know when ETS2LA is doing steering and acceleration overrides to detect ETS2LA usage!)
    ```python
    import mmap
    buf = mmap.mmap(0, 6, r"Local\ETS2LAPluginStatus")
    while True:
        format = "=i??"
        data = struct.unpack(format, buf[:2])
        data # (version, is_steering_overridden, is_acceleration_overridden)
    ```
- [x] Expose a virtual memory file to `Local\ETS2LAPluginInput` to **send input commands** directly into the game memory.
    ```python
    import time, mmap
    buf = mmap.mmap(0, 18, r"Local\ETS2LAPluginInput")
    while True:
        buf[:] = struct.pack('=f?ff?f',
            0.00, # Steering value
            True, # Override Steering
            time.time(), # Current timestamp for steering (data over 1s old will be ignored)
            0.00, # Acceleration (-1 - 1, brake - throttle) value
            True, # Override Acceleration
            time.time() # Current timestamp for acceleration (data over 1s old will be ignored)
        )
    ```
- [x] Expose a virtual memory file at `Local\ETS2LACameraProps` to get the **current camera properties** from the game.
    ```python
    import time, mmap
    buf = mmap.mmap(0, 128, r"Local\ETS2LACameraProps")
    while True:
        format = "=ffffhhffffffffffffffffffff"
        data = struct.unpack(format, buf[:36])
        data # fov, x, y, z, cx, cz, qw, qx, qy, qz, m11, m12, m13, m14, m21, ..., m44, truck_x, truck_y, truck_z, truck_rot_w, truck_rot_x, truck_rot_y, truck_rot_z
             # the m values are a 4x4 camera projection matrix.
             # the truck position (and rotation) values are interpolated to the current camera timestamp
             # if you want to render something at the truck position, use these values instead of the telemetry to avoid jitter
             # you can also just read the first 100 bytes to get the camera properties without the additional truck position and rotation data
    ```
- [x] Expose a virtual memory file at `Local\ETS2LATraffic` to get the **closest 40 traffic vehicles**.
    ```python
    import time, mmap
    buf = mmap.mmap(0, 6960, r"Local\ETS2LATraffic")
    while True:
        vehicle_format = "ffffffffffffhhbb"
        trailer_format = "ffffffffff"
        vehicle_object_format = vehicle_format + trailer_format + trailer_format + trailer_format
        total_format = "=" + vehicle_object_format * 40
        data = struct.unpack(total_format, buf[:6960])
        data # (vehicle + 3x trailer) * 40
        # vehicle:
        # x, y, z, qw, qx, qy, qz, width, height, length, speed, acceleration, trailer_count, id, is_tmp, is_trailer
        # trailer:
        # x, y, z, qw, qx, qy, qz, width, height, length
    ```

- [x] Expose a virtual memory file at `Local\ETS2LAParkedVehicles` to get the **closest 40 traffic parked vehicles**.
    ```python
    import time, mmap
    buf = mmap.mmap(0, 1720, r"Local\ETS2LAParkedVehicles")
    while True:
        vehicle_format = "ffffffffffhb"
        total_format = "=" + vehicle_format * 40
        data = struct.unpack(total_format, buf[:1720])
        data # vehicle * 40
        # x, y, z, qw, qx, qy, qz, width, height, length, id, is_trailer
    ```

- [x] Expose a virtual memory file at `Local\ETS2LASemaphore` to get the **closest 40 traffic lights and gates**.
    ```python
    import time, mmap
    buf = mmap.mmap(0, 1920, r"Local\ETS2LASemaphore")

    # Traffic light states
    OFF = 0
    ORANGE_TO_RED = 1
    RED = 2
    ORANGE_TO_GREEN = 4
    GREEN = 8
    SLEEP = 32

    # Gate states
    CLOSING = 0
    CLOSED = 1
    OPENING = 2
    OPEN = 3

    while True:
        semaphore_format = "fffhhffffifii"
        total_format = "=" + semaphore_format * 40
        data = struct.unpack(total_format, buf[:1920])
        data # (semaphore_format) * 40
        # traffic_light (value 9 == 1):
        # x, y, z, cx, cz, qw, qx, qy, qz, type, time_remaining (in state), state, id
        # gate (value 9 == 2):
        # x, y, z, cx, cz, qw, qx, qy, qz, type, time_remaining (in state), state, id
    ```
- [x] Expose a virtual memory file at `Local\ETS2LARoute` to output the **current navigation route**. NOTE: Limited to 6000 items! (mods might go over)
    ```python
    import time, mmap
    buf = mmap.mmap(0, 80_000, r"Local\ETS2LARoute")

    while True:
        item_format = "qff"
        total_format = "=" + item_format * 6000
        data = struct.unpack(total_format, buf[:96_000])
        data # (item_format) * 20
        # route item:
        # node_uid, distance_to_end (m), time_to_end (s)
    ```

# Credits
- **dariowouters** - Massive help and the creator of the original plugin.
- **ziakhan4505** - Has been developing our side of the plugin with the help of dariowouters.
- **Tumppi066** - Virtual memory implementation.
- **luuukske** - Our force feedback implementation is based on that luuukske made for MonoCruise. You can find that at https://github.com/luuukske/MonoCruise.