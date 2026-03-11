package com.dand104.syslogkitsend;

import javafx.application.Platform;
import javafx.fxml.FXML;
import javafx.scene.control.ComboBox;
import javafx.scene.control.Label;
import javafx.scene.control.TextArea;
import javafx.scene.control.TextField;
import javafx.scene.paint.Color;

import java.io.OutputStream;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.CompletableFuture;

public class MainController {

    @FXML private TextField hostField;
    @FXML private TextField portField;
    @FXML private ComboBox<String> protocolBox;
    @FXML private TextArea messageArea;
    @FXML private Label statusLabel;

    @FXML
    public void initialize() {
        protocolBox.getItems().addAll("UDP", "TCP");
        protocolBox.setValue("UDP");
    }

    @FXML
    protected void onSendButtonClick() {
        String host = hostField.getText().trim();
        String portStr = portField.getText().trim();
        String protocol = protocolBox.getValue();
        String message = messageArea.getText().trim();

        if (host.isEmpty() || portStr.isEmpty() || message.isEmpty()) {
            setStatus("Fill all fields", Color.RED);
            return;
        }

        int port;
        try {
            port = Integer.parseInt(portStr);
        } catch (NumberFormatException e) {
            setStatus("Port must be a number", Color.RED);
            return;
        }

        setStatus("Sending...", Color.BLUE);

        CompletableFuture.runAsync(() -> {
            try {
                if ("UDP".equals(protocol)) {
                    sendUdp(host, port, message);
                } else {
                    sendTcp(host, port, message);
                }

                Platform.runLater(() -> setStatus("Message sent: " + protocol, Color.GREEN));

            } catch (Exception e) {
                Platform.runLater(() -> setStatus("Error occurred: " + e.getMessage(), Color.RED));
            }
        });
    }

    private void sendUdp(String host, int port, String message) throws Exception {
        try (DatagramSocket socket = new DatagramSocket()) {
            InetAddress address = InetAddress.getByName(host);
            byte[] buf = message.getBytes(StandardCharsets.UTF_8);
            DatagramPacket packet = new DatagramPacket(buf, buf.length, address, port);
            socket.send(packet);
        }
    }

    private void sendTcp(String host, int port, String message) throws Exception {
        try (Socket socket = new Socket()) {
            socket.connect(new InetSocketAddress(host, port), 3000);
            OutputStream out = socket.getOutputStream();
            out.write((message + "\n").getBytes(StandardCharsets.UTF_8));
            out.flush();
        }
    }

    private void setStatus(String text, Color color) {
        statusLabel.setText(text);
        statusLabel.setTextFill(color);
    }
}