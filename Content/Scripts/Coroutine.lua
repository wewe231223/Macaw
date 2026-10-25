function Wait(seconds)
    coroutine.yield("wait", seconds)
end

function WaitUntil(condition)
    coroutine.yield("wait_until", condition)
end

