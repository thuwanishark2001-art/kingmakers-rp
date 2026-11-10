package com.russia.game.gui

import com.russia.game.R
import com.russia.game.core.Samp.Companion.activity
import com.russia.game.databinding.BusStopBinding

class BusStation : NativeGui<BusStopBinding>(BusStopBinding::class) {

    fun update(time: Int) {
        activity.runOnUiThread {
            binding.timeText.text = String.format("%d", time)
        }
    }

    override fun receivePacket(actionId: Int, data: String) {
        // No server packets handled by this screen yet; ignore instead of crashing.
    }
}