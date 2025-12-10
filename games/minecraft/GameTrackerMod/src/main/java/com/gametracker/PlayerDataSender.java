package com.gametracker; // <-- IMPORTANT: Change this to your package

import com.mojang.logging.LogUtils;
import net.minecraft.client.Minecraft;
import net.minecraft.world.entity.player.Player;
import net.neoforged.api.distmarker.Dist;
import net.neoforged.bus.api.SubscribeEvent;
import net.neoforged.fml.common.EventBusSubscriber;
import net.neoforged.neoforge.client.event.ClientTickEvent;
import org.slf4j.Logger;

import java.io.IOException;
import java.io.OutputStream;
import java.io.PrintWriter;
import java.net.Socket;
import java.net.SocketTimeoutException;
import java.net.UnknownHostException;

/**
 * This class is responsible for sending player data from the Minecraft client
 * to an external Python server every second.
 */
@EventBusSubscriber(Dist.CLIENT)
public class PlayerDataSender {

    private static final Logger LOGGER = LogUtils.getLogger();

    // --- Configuration ---
    // The address of the Python server. Use "127.0.0.1" or "localhost" if it's on the same machine.
    private static final String PYTHON_SERVER_HOST = "127.0.0.1";
    // The port must match the one in the Python script.
    private static final int PYTHON_SERVER_PORT = 9999;
    // ---------------------

    private static int tickCounter = 0;
    // There are 20 ticks in a second. We want to send data every 20 ticks.
    private static final int TICKS_PER_SEND = 20;

    /**
     * This event listener is called on every client tick.
     * We use it to count ticks and trigger our data-sending task once per second.
     */
    @SubscribeEvent
    public static void onClientTick(ClientTickEvent.Post event) {
        // We only run our logic on the POST phase of the tick to ensure the game state is stable.
        tickCounter++;
        if (tickCounter >= TICKS_PER_SEND) {
            tickCounter = 0;
            sendDataToPython();
        }
    }

    /**
     * Gathers player data and sends it to the Python server in a separate thread.
     * Running network operations on a separate thread is crucial to prevent
     * the main game thread from freezing (causing lag).
     */
    private static void sendDataToPython() {
        // Use a separate thread for network operations to avoid freezing the game
        Thread networkThread = new Thread(() -> {
            Minecraft mc = Minecraft.getInstance();
            Player player = mc.player;

            // If the player is not in a world (e.g., on the main menu), do nothing.
            if (player == null) {
                return;
            }

            // --- Gather Player Data ---
            String playerName = player.getName().getString();
            double x = player.getX();
            double y = player.getY();
            double z = player.getZ();
            float health = player.getHealth();
            String dimension = player.level().dimension().location().toString();

            // --- Format as JSON String ---
            // This is a simple way to create a JSON string. For more complex objects,
            // consider using a library like Gson or Jackson.
            String jsonData = String.format(
                    "{\"playerName\":\"%s\",\"x\":%.2f,\"y\":%.2f,\"z\":%.2f,\"health\":%.1f,\"dimension\":\"%s\"}",
                    playerName, x, y, z, health, dimension
            );

            // --- Send Data ---
            try (Socket socket = new Socket(PYTHON_SERVER_HOST, PYTHON_SERVER_PORT)) {
                // Set a timeout for the connection and read operations (2 seconds)
                socket.setSoTimeout(2000);

                OutputStream output = socket.getOutputStream();
                // PrintWriter with 'true' for auto-flush sends the data immediately.
                PrintWriter writer = new PrintWriter(output, true);

                LOGGER.info("[DataSender] Sending data to Python server.");
                writer.println(jsonData);

            } catch (UnknownHostException e) {
                LOGGER.error("[DataSender] Unknown Python server host: {}", PYTHON_SERVER_HOST);
            } catch (SocketTimeoutException e) {
                LOGGER.error("[DataSender] Connection to Python server timed out.");
            } catch (IOException e) {
                // This is expected if the Python server isn't running. We log it but don't crash.
                LOGGER.error("[DataSender] Could not connect to Python server at {}:{}.", PYTHON_SERVER_HOST, PYTHON_SERVER_PORT);
                LOGGER.error("[DataSender] Please ensure the Python script is running and the port is correct.");
            } catch (Exception e) {
                LOGGER.error("[DataSender] An unexpected error occurred while sending data.", e);
            }
        });

        // Set the thread as a daemon so it doesn't prevent the game from closing.
        networkThread.setDaemon(true);
        networkThread.start();
    }
}