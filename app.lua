print("Lua script loaded successfully!")

local frame_count = 0

-- This function is called by C every frame
function update()
    frame_count = frame_count + 1
    
    -- Print a message every 60 frames (approx 1 sec)
    if frame_count % 60 == 0 then
        print("Lua update running... Frame: " .. frame_count)
    end
end