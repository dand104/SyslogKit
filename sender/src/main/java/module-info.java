module com.dand104.syslogkitsend {
    requires javafx.controls;
    requires javafx.fxml;

    requires org.controlsfx.controls;

    opens com.dand104.syslogkitsend to javafx.fxml;
    exports com.dand104.syslogkitsend;
}