/* ESPDMXConfig.java
 * (part of LXDMXWiFiLibrary)
 *
 * Copyright 2017-2018 Claude Heintz Design, All rights reserved
 *
 * see https://www.claudeheintzdesign.com/lx/opensource.html for license
 *
 * Version 2.0 for RDM compatible interfaces
 *
 */

import javax.swing.*;
import javax.swing.table.AbstractTableModel;

import java.awt.*;
import java.awt.event.*;
import java.io.FileOutputStream;
import java.io.PrintStream;
import java.net.*;
import java.util.*;
import java.util.List;

public class ESPDMXConfig extends JFrame {

	private static final long serialVersionUID = 1L;
	private static final boolean SYSTEM_OUT2LOG = true;
	
	public static final int ESP_CONFIG_PKT_SIZE = 232;
	
	LXESPDMXDiscoverer discoverer = null;
	ESPDMXConfigPacketList configList;
	
	JTabbedPane _tabs;
	JPanel _searchPanel;
	JPanel _configPanel;

	ButtonGroup _bgWifiMode;
	JRadioButton _jrbAccessPoint;
	JRadioButton _jrbStation;

	JTextField _jtfSSID;
	JTextField _jtfPWD;

	JTextField _jtfAccessPointIP;
	JTextField _jtfAccessPointGateway;
	JTextField _jtfAccessPointSubnet;

	JTextField _jtfStationIP;
	JTextField _jtfStationGateway;
	JTextField _jtfStationSubnet;

	JTextField _jtfMulticastAddress;
	JTextField _jtfSACNUniverse;

	JTextField _jtfArtNetPANet;
	JTextField _jtfArtNetSubnet;
	JTextField _jtfArtNetUniverse;
	JTextField _jtfArtNetNodeName;
	
	JTextField _jtfDeviceAddress;
	JTextField _jtfInputNetTarget;

	ButtonGroup _bgStaticDHCP;
	JRadioButton _jrbStatic;
	JRadioButton _jrbDHCP;
	ButtonGroup _bgProtocol;
	JRadioButton _jrbSACN;
	JRadioButton _jrbArtNet;
	
	ButtonGroup _bgMultiBroadcast;
	JRadioButton _jrbMulticast;
	JRadioButton _jrbBroadcast;
	
	ButtonGroup _bgRDMDMX;
	JRadioButton _jrbRDM;
	JRadioButton _jrbDMX;
	
	ButtonGroup _bgDMXMode;
	JRadioButton _jrbDMXOutput;
	JRadioButton _jrbDMXInput;

	ButtonGroup _bgTargetPort;
	JRadioButton _jrbTargetSACN;
	JRadioButton _jrbTargetArtNet;

	JTextField _jtfNetInterface;
	JTextField _jtfInterfaceAddress;
	javax.swing.JLabel _jLabelProgress;
	
	JComboBox<String> _jcbTargetSelect;
	
	JTable _jtable;

	JButton _jbClearOutput;
	JButton _jbCancelMerge;
	JButton _jbSearch;
	JButton _jbGetInfo;
	JButton _jbUpload;
	JButton _jbReset;
	JButton _jbDone;
	
	public static byte OPCODE_DATA = 0;
	public static byte OPCODE_QUERY = '?';
	public static byte OPCODE_UPLOAD = '!';
	public static byte OPCODE_RESET = '^';
	
	public ESPDMXConfig() {
		redirectSystemOut();
		configList = new ESPDMXConfigPacketList();
		initInterface();
	}

	public ESPDMXConfig(String args) {
		redirectSystemOut();
		configList = new ESPDMXConfigPacketList();
		initInterface();
	}

	public static void resizeAndCenter(int sx, int sy, Frame tf) {
		Dimension sc = tf.getToolkit().getScreenSize();
		sx = (sx>sc.getWidth()) ? (int) sc.getWidth() : sx;
		sy = (sy>sc.getHeight()) ? (int) sc.getHeight() : sy;
		tf.setSize(new java.awt.Dimension(sx, sy));
		int x = (int) (sc.getWidth()/2-tf.getWidth()/2);
		int y = (int) (sc.getHeight()/2-tf.getHeight()/2);
		tf.setLocation(new java.awt.Point(x, y));
		tf.validate();
	}

	public void initInterface() {
		setTitle("ESP-DMX Configuration Utility");
		resizeAndCenter(680,600,this);

		addWindowListener(new java.awt.event.WindowAdapter() {
			public void windowClosing(java.awt.event.WindowEvent e) {
				exit();
			}
		});

		_tabs = new JTabbedPane();
		add(_tabs);
		_searchPanel = new JPanel();
		_configPanel = new JPanel();
		_tabs.addTab("Search", null, _searchPanel);
		
		_searchPanel.setLayout(null);
		_configPanel.setLayout(null);

		_jtfSSID = new javax.swing.JTextField();
		_jtfSSID.setSize(new java.awt.Dimension(275, 25));
		_jtfSSID.setText( "" );
		_jtfSSID.setLocation(new java.awt.Point(110, 10));
		_configPanel.add(_jtfSSID);

		javax.swing.JLabel jLabel1 = new javax.swing.JLabel();
		jLabel1.setSize(new java.awt.Dimension(80, 20));
		jLabel1.setVisible(true);
		jLabel1.setText("SSID:");
		jLabel1.setHorizontalAlignment(SwingConstants.RIGHT);
		jLabel1.setLocation(new java.awt.Point(28, 12));
		_configPanel.add(jLabel1);

		_jtfPWD = new javax.swing.JTextField();
		_jtfPWD.setSize(new java.awt.Dimension(275, 25));
		_jtfPWD.setText( "" );
		_jtfPWD.setLocation(new java.awt.Point(110, 40));
		_configPanel.add(_jtfPWD);

		javax.swing.JLabel jLabel2 = new javax.swing.JLabel();
		jLabel2.setSize(new java.awt.Dimension(80, 20));
		jLabel2.setVisible(true);
		jLabel2.setText("Password:");
		jLabel2.setHorizontalAlignment(SwingConstants.RIGHT);
		jLabel2.setLocation(new java.awt.Point(28, 42));
		_configPanel.add(jLabel2);

		_jrbAccessPoint = new JRadioButton("Access Point");
		_jrbAccessPoint.setLocation(new Point(110, 80));
		_jrbAccessPoint.setSize(new Dimension(150, 20));
		_jrbAccessPoint.setSelected(true);

		_jrbStation = new JRadioButton("Station");
		_jrbStation.setLocation(new Point(350, 80));
		_jrbStation.setSize(new Dimension(100, 20));

		_bgWifiMode = new ButtonGroup();
		_bgWifiMode.add(_jrbAccessPoint);
		_bgWifiMode.add(_jrbStation);
		_configPanel.add(_jrbAccessPoint);
		_configPanel.add(_jrbStation);

		// ---------- ap fields & labels

		_jtfAccessPointIP = new javax.swing.JTextField();
		_jtfAccessPointIP.setSize(new java.awt.Dimension(130, 25));
		_jtfAccessPointIP.setText( "" );
		_jtfAccessPointIP.setLocation(new java.awt.Point(110, 130));
		_configPanel.add(_jtfAccessPointIP);

		javax.swing.JLabel jLabel3 = new javax.swing.JLabel();
		jLabel3.setSize(new java.awt.Dimension(80, 20));
		jLabel3.setVisible(true);
		jLabel3.setText("AP Address:");
		jLabel3.setHorizontalAlignment(SwingConstants.RIGHT);
		jLabel3.setLocation(new java.awt.Point(28, 132));
		_configPanel.add(jLabel3);

		_jtfAccessPointGateway = new javax.swing.JTextField();
		_jtfAccessPointGateway.setSize(new java.awt.Dimension(130, 25));
		_jtfAccessPointGateway.setText( "" );
		_jtfAccessPointGateway.setLocation(new java.awt.Point(110, 160));
		_configPanel.add(_jtfAccessPointGateway);

		javax.swing.JLabel jLabel4 = new javax.swing.JLabel();
		jLabel4.setSize(new java.awt.Dimension(80, 20));
		jLabel4.setVisible(true);
		jLabel4.setText("AP Gateway:");
		jLabel4.setHorizontalAlignment(SwingConstants.RIGHT);
		jLabel4.setLocation(new java.awt.Point(28, 162));
		_configPanel.add(jLabel4);

		_jtfAccessPointSubnet = new javax.swing.JTextField();
		_jtfAccessPointSubnet.setSize(new java.awt.Dimension(130, 25));
		_jtfAccessPointSubnet.setText( "" );
		_jtfAccessPointSubnet.setLocation(new java.awt.Point(110, 190));
		_configPanel.add(_jtfAccessPointSubnet);

		javax.swing.JLabel jLabel5 = new javax.swing.JLabel();
		jLabel5.setSize(new java.awt.Dimension(80, 20));
		jLabel5.setVisible(true);
		jLabel5.setText("AP Subnet:");
		jLabel5.setHorizontalAlignment(SwingConstants.RIGHT);
		jLabel5.setLocation(new java.awt.Point(28, 192));
		_configPanel.add(jLabel5);

		// ---------- station fields & labels
		
		_jrbStatic = new JRadioButton("Static");
		_jrbStatic.setLocation(new Point(395, 105));
		_jrbStatic.setSize(new Dimension(100, 20));
		_jrbStatic.setSelected(true);

		_jrbDHCP = new JRadioButton("DHCP");
		_jrbDHCP.setLocation(new Point(530, 105));
		_jrbDHCP.setSize(new Dimension(100, 20));

		_bgStaticDHCP = new ButtonGroup();
		_bgStaticDHCP.add(_jrbStatic);
		_bgStaticDHCP.add(_jrbDHCP);
		_configPanel.add(_jrbStatic);
		_configPanel.add(_jrbDHCP);

		_jtfStationIP = new javax.swing.JTextField();
		_jtfStationIP.setSize(new java.awt.Dimension(130, 25));
		_jtfStationIP.setText( "" );
		_jtfStationIP.setLocation(new java.awt.Point(375, 130));
		_configPanel.add(_jtfStationIP);

		javax.swing.JLabel jLabel6 = new javax.swing.JLabel();
		jLabel6.setSize(new java.awt.Dimension(80, 20));
		jLabel6.setVisible(true);
		jLabel6.setText("Address:");
		jLabel6.setHorizontalAlignment(SwingConstants.RIGHT);
		jLabel6.setLocation(new java.awt.Point(292, 132));
		_configPanel.add(jLabel6);

		_jtfStationGateway = new javax.swing.JTextField();
		_jtfStationGateway.setSize(new java.awt.Dimension(130, 25));
		_jtfStationGateway.setText( "" );
		_jtfStationGateway.setLocation(new java.awt.Point(375, 160));
		_configPanel.add(_jtfStationGateway);

		javax.swing.JLabel jLabel7 = new javax.swing.JLabel();
		jLabel7.setSize(new java.awt.Dimension(80, 20));
		jLabel7.setVisible(true);
		jLabel7.setText("Gateway:");
		jLabel7.setHorizontalAlignment(SwingConstants.RIGHT);
		jLabel7.setLocation(new java.awt.Point(292, 162));
		_configPanel.add(jLabel7);

		_jtfStationSubnet = new javax.swing.JTextField();
		_jtfStationSubnet.setSize(new java.awt.Dimension(130, 25));
		_jtfStationSubnet.setText( "" );
		_jtfStationSubnet.setLocation(new java.awt.Point(375, 190));
		_configPanel.add(_jtfStationSubnet);

		javax.swing.JLabel jLabel8 = new javax.swing.JLabel();
		jLabel8.setSize(new java.awt.Dimension(80, 20));
		jLabel8.setVisible(true);
		jLabel8.setText("Subnet:");
		jLabel8.setHorizontalAlignment(SwingConstants.RIGHT);
		jLabel8.setLocation(new java.awt.Point(292, 192));
		_configPanel.add(jLabel8);

		// ---------- sacn text fields
		
		JSeparator protocolSeparator = new JSeparator(SwingConstants.HORIZONTAL);
		protocolSeparator.setLocation(new Point(10, 225));
		protocolSeparator.setSize(new Dimension(640, 20));
		_configPanel.add(protocolSeparator);

		_jtfMulticastAddress = new javax.swing.JTextField();
		_jtfMulticastAddress.setSize(new java.awt.Dimension(130, 25));
		_jtfMulticastAddress.setText( "" );
		_jtfMulticastAddress.setLocation(new java.awt.Point(110, 280));
		_configPanel.add(_jtfMulticastAddress);

		javax.swing.JLabel jLabel9 = new javax.swing.JLabel();
		jLabel9.setSize(new java.awt.Dimension(80, 20));
		jLabel9.setVisible(true);
		jLabel9.setText("Multicast IP:");
		jLabel9.setHorizontalAlignment(SwingConstants.RIGHT);
		jLabel9.setLocation(new java.awt.Point(28, 282));
		_configPanel.add(jLabel9);

		_jtfSACNUniverse = new javax.swing.JTextField();
		_jtfSACNUniverse.setSize(new java.awt.Dimension(40, 25));
		_jtfSACNUniverse.setText( "" );
		_jtfSACNUniverse.setLocation(new java.awt.Point(110, 250));
		_configPanel.add(_jtfSACNUniverse);

		javax.swing.JLabel jLabel10 = new javax.swing.JLabel();
		jLabel10.setSize(new java.awt.Dimension(100, 20));
		jLabel10.setVisible(true);
		jLabel10.setText("sACN Universe:");
		jLabel10.setHorizontalAlignment(SwingConstants.RIGHT);
		jLabel10.setLocation(new java.awt.Point(8, 252));
		_configPanel.add(jLabel10);
		
		_jrbMulticast = new JRadioButton("Multicast");
		_jrbMulticast.setLocation(new Point(130, 312));
		_jrbMulticast.setSize(new Dimension(100, 20));

		_jrbBroadcast = new JRadioButton("Unicast");
		_jrbBroadcast.setLocation(new Point(25, 312));
		_jrbBroadcast.setSize(new Dimension(160, 20));
		_jrbBroadcast.setSelected(true);

		_bgMultiBroadcast = new ButtonGroup();
		_bgMultiBroadcast.add(_jrbMulticast);
		_bgMultiBroadcast.add(_jrbBroadcast);
		_configPanel.add(_jrbMulticast);
		_configPanel.add(_jrbBroadcast);

		// ---------- Art-Net text fields
		
		_jtfArtNetPANet = new javax.swing.JTextField();
		_jtfArtNetPANet.setSize(new java.awt.Dimension(40, 25));
		_jtfArtNetPANet.setText( "" );
		_jtfArtNetPANet.setLocation(new java.awt.Point(375, 250));
		_configPanel.add(_jtfArtNetPANet);

		_jtfArtNetSubnet = new javax.swing.JTextField();
		_jtfArtNetSubnet.setSize(new java.awt.Dimension(40, 25));
		_jtfArtNetSubnet.setText( "" );
		_jtfArtNetSubnet.setLocation(new java.awt.Point(415, 250));
		_configPanel.add(_jtfArtNetSubnet);

		javax.swing.JLabel jLabel11 = new javax.swing.JLabel();
		jLabel11.setSize(new java.awt.Dimension(200, 20));
		jLabel11.setVisible(true);
		jLabel11.setText("Art-Net Port Address:");
		jLabel11.setHorizontalAlignment(SwingConstants.RIGHT);
		jLabel11.setLocation(new java.awt.Point(172, 252));
		_configPanel.add(jLabel11);

		_jtfArtNetUniverse = new javax.swing.JTextField();
		_jtfArtNetUniverse.setSize(new java.awt.Dimension(40, 25));
		_jtfArtNetUniverse.setText( "" );
		_jtfArtNetUniverse.setLocation(new java.awt.Point(455, 250));
		_configPanel.add(_jtfArtNetUniverse);

		javax.swing.JLabel jLabel12 = new javax.swing.JLabel();
		jLabel12.setSize(new java.awt.Dimension(200, 20));
		jLabel12.setVisible(true);
		jLabel12.setText("[Net]-[Subnet]-[Universe]");
		jLabel12.setHorizontalAlignment(SwingConstants.RIGHT);
		String cfname = jLabel12.getFont().getName();
		jLabel12.setFont(new Font(cfname, 0, 10));
		jLabel12.setLocation(new java.awt.Point(295, 278));
		_configPanel.add(jLabel12);

		_jtfArtNetNodeName = new javax.swing.JTextField();
		_jtfArtNetNodeName.setSize(new java.awt.Dimension(275, 25));
		_jtfArtNetNodeName.setText( "" );
		_jtfArtNetNodeName.setLocation(new java.awt.Point(375, 310));
		_configPanel.add(_jtfArtNetNodeName);

		javax.swing.JLabel jLabel13 = new javax.swing.JLabel();
		jLabel13.setSize(new java.awt.Dimension(120, 20));
		jLabel13.setVisible(true);
		jLabel13.setText("Node Name:");
		jLabel13.setHorizontalAlignment(SwingConstants.RIGHT);
		jLabel13.setLocation(new java.awt.Point(252, 312));
		_configPanel.add(jLabel13);
		
		_jbClearOutput = new JButton("Clear Output");
		_jbClearOutput.setSize(new Dimension(120, 25));
		_jbClearOutput.setLocation(new Point(530, 280));
		_jbClearOutput.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				sendArtAddressCommand(targetIP(), 0x90);
			}
		});
		_configPanel.add(_jbClearOutput);

		_jbCancelMerge = new JButton("Cancel Merge");
		_jbCancelMerge.setSize(new Dimension(120, 25));
		_jbCancelMerge.setLocation(new Point(530, 250));
		_jbCancelMerge.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				sendArtAddressCommand(targetIP(), 0x01);
			}
		});
		_configPanel.add(_jbCancelMerge);
		
		// ---------- output or input  mode selection
		
		JSeparator modeSeparator = new JSeparator(SwingConstants.HORIZONTAL);
		modeSeparator.setLocation(new Point(10, 345));
		modeSeparator.setSize(new Dimension(640, 20));
		_configPanel.add(modeSeparator);
		
		_jrbDMXOutput = new JRadioButton("DMX Output");
		_jrbDMXOutput.setLocation(new Point(25, 375));
		_jrbDMXOutput.setSize(new Dimension(150, 20));
		_jrbDMXOutput.setSelected(true);

		_jrbDMXInput = new JRadioButton("DMX Input");
		_jrbDMXInput.setLocation(new Point(25, 425));
		_jrbDMXInput.setSize(new Dimension(160, 20));

		_bgDMXMode = new ButtonGroup();
		_bgDMXMode.add(_jrbDMXOutput);
		_bgDMXMode.add(_jrbDMXInput);
		_configPanel.add(_jrbDMXOutput);
		_configPanel.add(_jrbDMXInput);
		
		
		_jtfDeviceAddress = new javax.swing.JTextField();
		_jtfDeviceAddress.setSize(new java.awt.Dimension(60, 25));
		_jtfDeviceAddress.setText( "" );
		_jtfDeviceAddress.setLocation(new java.awt.Point(300, 373));
		_configPanel.add(_jtfDeviceAddress);

		javax.swing.JLabel jLabel16 = new javax.swing.JLabel();
		jLabel16.setSize(new java.awt.Dimension(125, 20));
		jLabel16.setVisible(true);
		jLabel16.setText("Device Address:");
		jLabel16.setHorizontalAlignment(SwingConstants.RIGHT);
		jLabel16.setLocation(new java.awt.Point(172, 375));
		_configPanel.add(jLabel16);
		
		_jtfInputNetTarget = new javax.swing.JTextField();
		_jtfInputNetTarget.setSize(new java.awt.Dimension(130, 25));
		_jtfInputNetTarget.setText( "" );
		_jtfInputNetTarget.setLocation(new java.awt.Point(230, 423));
		_configPanel.add(_jtfInputNetTarget);

		javax.swing.JLabel jLabel19 = new javax.swing.JLabel();
		jLabel19.setSize(new java.awt.Dimension(120, 20));
		jLabel19.setVisible(true);
		jLabel19.setText("to net:");
		jLabel19.setHorizontalAlignment(SwingConstants.RIGHT);
		jLabel19.setLocation(new java.awt.Point(107, 425));
		_configPanel.add(jLabel19);
		
		
		_jrbRDM = new JRadioButton("RDM");
		_jrbRDM.setLocation(new Point(485, 375));
		_jrbRDM.setSize(new Dimension(100, 20));

		_jrbDMX = new JRadioButton("DMX Only");
		_jrbDMX.setLocation(new Point(380, 375));
		_jrbDMX.setSize(new Dimension(100, 20));
		_jrbDMX.setSelected(true);
		
		_bgRDMDMX = new ButtonGroup();
		_bgRDMDMX.add(_jrbRDM);
		_bgRDMDMX.add(_jrbDMX);
		_configPanel.add(_jrbRDM);
		_configPanel.add(_jrbDMX);
		
		
		// ---------- input protocol options

		_jrbSACN = new JRadioButton("sACN");
		_jrbSACN.setLocation(new Point(485, 425));
		_jrbSACN.setSize(new Dimension(100, 20));

		_jrbArtNet = new JRadioButton("Art-Net");
		_jrbArtNet.setLocation(new Point(380, 425));
		_jrbArtNet.setSize(new Dimension(100, 20));
		_jrbArtNet.setSelected(true);

		_bgProtocol = new ButtonGroup();
		_bgProtocol.add(_jrbSACN);
		_bgProtocol.add(_jrbArtNet);
		_configPanel.add(_jrbSACN);
		_configPanel.add(_jrbArtNet);
				
				
		// ---------- Done / Upload buttons
		
		_jbUpload = new JButton("Done");
		_jbUpload.setSize(new Dimension(120, 30));
		_jbUpload.setLocation(new Point(500, 475));
		_jbUpload.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				setSearchTab();
			}
		});
		_configPanel.add(_jbUpload);
		
		_jbUpload = new JButton("Upload");
		_jbUpload.setSize(new Dimension(120, 30));
		_jbUpload.setLocation(new Point(170, 475));
		_jbUpload.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				int result = upload();
				if ( result < 0 ) {
					alert("Upload failed");
				} else if ( result == 0 ) {
					configList.reset();
					setSearchTab();
				}
			}
		});
		_configPanel.add(_jbUpload);
		
		_jbReset = new JButton("Send Reset");
		_jbReset.setSize(new Dimension(120, 30));
		_jbReset.setLocation(new Point(30, 475));
		_jbReset.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				sendResetPacket(targetIP(), getTargetUDPPort());
				configList.reset();
				setSearchTab();
			}
		});
		_configPanel.add(_jbReset);

		//***************************************************************************************************
		// ****************************************** search panel ******************************************

		_jtable = new JTable();
		_jtable.setModel(configList);
		
		JScrollPane jsp = new JScrollPane(_jtable);
		jsp.setVisible(true);
		jsp.setSize(new java.awt.Dimension(350, 250));
		jsp.setLocation(new Point(150,30));
		_searchPanel.add(jsp);

		//  target stuff 
		
		_jtfNetInterface = new javax.swing.JTextField();
		_jtfNetInterface.setSize(new java.awt.Dimension(130, 25));
		_jtfNetInterface.setText( "en1" );
		_jtfNetInterface.setLocation(new java.awt.Point(180, 350));
		_searchPanel.add(_jtfNetInterface);

		javax.swing.JLabel jLabel17 = new javax.swing.JLabel();
		jLabel17.setSize(new java.awt.Dimension(150, 20));
		jLabel17.setVisible(true);
		jLabel17.setText("Network Interface:");
		jLabel17.setHorizontalAlignment(SwingConstants.RIGHT);
		jLabel17.setLocation(new java.awt.Point(28, 352));
		_searchPanel.add(jLabel17);
		
		_jtfInterfaceAddress = new javax.swing.JTextField();
		_jtfInterfaceAddress.setSize(new java.awt.Dimension(130, 25));
		_jtfInterfaceAddress.setText( "any" );
		_jtfInterfaceAddress.setLocation(new java.awt.Point(180, 380));
		_searchPanel.add(_jtfInterfaceAddress);

		javax.swing.JLabel jLabel18 = new javax.swing.JLabel();
		jLabel18.setSize(new java.awt.Dimension(150, 20));
		jLabel18.setVisible(true);
		jLabel18.setText("Interface IP Address:");
		jLabel18.setHorizontalAlignment(SwingConstants.RIGHT);
		jLabel18.setLocation(new java.awt.Point(28, 382));
		_searchPanel.add(jLabel18);

		_jcbTargetSelect = new JComboBox<String>();
		_jcbTargetSelect.addItem("Default Access Point");
		_jcbTargetSelect.addItem("10.110.115.255");
		_jcbTargetSelect.addItem("10.255.255.255");
		_jcbTargetSelect.addItem("192.168.1.255");
		_jcbTargetSelect.setSize(new Dimension(200, 25));
		_jcbTargetSelect.setLocation(new Point(110, 430));
		_jcbTargetSelect.setEditable(true);
		_searchPanel.add(_jcbTargetSelect);

		javax.swing.JLabel jLabel14 = new javax.swing.JLabel();
		jLabel14.setSize(new java.awt.Dimension(80, 20));
		jLabel14.setVisible(true);
		jLabel14.setText("Target IP:");
		jLabel14.setHorizontalAlignment(SwingConstants.RIGHT);
		jLabel14.setLocation(new java.awt.Point(28, 432));
		_searchPanel.add(jLabel14);

		_jrbTargetSACN = new JRadioButton("sACN");
		_jrbTargetSACN.setLocation(new Point(192, 465));
		_jrbTargetSACN.setSize(new Dimension(90, 20));

		_jrbTargetArtNet = new JRadioButton("Art-Net");
		_jrbTargetArtNet.setLocation(new Point(107, 465));
		_jrbTargetArtNet.setSize(new Dimension(100, 20));
		_jrbTargetArtNet.setSelected(true);
		
		javax.swing.JLabel jLabel15 = new javax.swing.JLabel();
		jLabel15.setSize(new java.awt.Dimension(80, 20));
		jLabel15.setVisible(true);
		jLabel15.setText("Target Port:");
		jLabel15.setHorizontalAlignment(SwingConstants.RIGHT);
		jLabel15.setLocation(new java.awt.Point(28, 465));
		_searchPanel.add(jLabel15);

		_bgTargetPort = new ButtonGroup();
		_bgTargetPort.add(_jrbTargetSACN);
		_bgTargetPort.add(_jrbTargetArtNet);
		_searchPanel.add(_jrbTargetSACN);
		_searchPanel.add(_jrbTargetArtNet);

		_jbGetInfo = new JButton("Show Info");
		_jbGetInfo.setSize(new Dimension(120, 30));
		_jbGetInfo.setLocation(new Point(375, 290));
		_jbGetInfo.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				ESPDMXConfigPacket cp = selectedPacket();
				if ( cp != null ) {
					loadPacket(cp);
					_jcbTargetSelect.setSelectedItem(cp.source().getHostAddress());
					setConfigureTab();
				}
			}
		});
		_jbGetInfo.setEnabled(false);
		_searchPanel.add(_jbGetInfo);
		
		_jLabelProgress = new javax.swing.JLabel();
		_jLabelProgress.setSize(new java.awt.Dimension(225, 20));
		_jLabelProgress.setVisible(true);
		_jLabelProgress.setText("");
		//_jLabelProgress.setHorizontalAlignment(SwingConstants.RIGHT);
		_jLabelProgress.setLocation(new java.awt.Point(375, 400));
		_searchPanel.add(_jLabelProgress);
		
		_jbSearch = new JButton("Search");
		_jbSearch.setSize(new Dimension(120, 30));
		_jbSearch.setLocation(new Point(375, 430));
		_jbSearch.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				_jbGetInfo.setEnabled(false);
				startDiscovery();		//wifi and any address
				if ( discoverer != null ) {
					sendQuery();
				}
			}
		});
		_searchPanel.add(_jbSearch);

		setVisible(true);
	}

	/**
	 * clean up and quit
	 */
	public void exit() {
		if ( discoverer != null ) {
			discoverer.close();	//end socket thread
		}
		System.out.print("Exited " + new java.util.Date());
		System.exit(0);
	}
	
	public void setConfigureTab() {
		_tabs.addTab("Configure", null, _configPanel);
		_tabs.remove(_searchPanel);
	}
	
	public void setSearchTab() {
		_tabs.addTab("Search", null, _searchPanel);
		_tabs.remove(_configPanel);
	}
	
	// ******************  query & receive  ******************
	
	public ESPDMXConfigPacket selectedPacket() {
		ESPDMXConfigPacket cp = null;
		int sr = _jtable.getSelectedRow();
		if ( sr >= 0 ) {
			cp = configList.packetAt(sr);
		}
		return cp;
	}

	/**
	 * Loads an ESPDMXConfigPacket into the interface
	 * @param cpacket
	 */
	public void loadPacket(ESPDMXConfigPacket cpacket) {
		_jtfSSID.setText(cpacket.ssid());
		_jtfPWD.setText(cpacket.password());
		_jtfArtNetNodeName.setText(cpacket.nodeName());

		if ( cpacket.stationMode() ) {
			_jrbStation.setSelected(true);
		} else {
			_jrbAccessPoint.setSelected(true);
		}

		if ( cpacket.artNetMode() ) {
			_jrbArtNet.setSelected(true);
		} else {
			_jrbSACN.setSelected(true);
		}

		if ( cpacket.dhcpMode()) {
			_jrbDHCP.setSelected(true);
		} else {
			_jrbStatic.setSelected(true);
		}

		if ( cpacket.multicastMode() ) {
			_jrbMulticast.setSelected(true);
		} else {
			_jrbBroadcast.setSelected(true);
		}
		
		if ( cpacket.outputMode() ) {
			_jrbDMXOutput.setSelected(true);
		} else {
			_jrbDMXInput.setSelected(true);
		}
		
		if ( cpacket.rdmMode() ) {
			_jrbRDM.setSelected(true);
		} else {
			_jrbDMX.setSelected(true);
		}

		_jtfAccessPointIP.setText(cpacket.apStaticAddress());
		_jtfAccessPointGateway.setText(cpacket.apGatewayAddress());
		_jtfAccessPointSubnet.setText(cpacket.apSubnetAddress());
		_jtfStationIP.setText(cpacket.stationStaticAddress());
		_jtfStationGateway.setText(cpacket.stationGatewayAddress());
		_jtfStationSubnet.setText(cpacket.stationSubnetAddress());
		_jtfMulticastAddress.setText(cpacket.multicastAddress());
		_jtfInputNetTarget.setText(cpacket.inputAddress());
		_jtfDeviceAddress.setText(Integer.toString(cpacket.deviceAddress()));

		_jtfSACNUniverse.setText(cpacket.sacnUniverse());
		_jtfArtNetPANet.setText(cpacket.artNetPortAddressNet());
		_jtfArtNetSubnet.setText(cpacket.artNetSubnet());
		_jtfArtNetUniverse.setText(cpacket.artNetUniverse());
	}
	
	/**
	 * Called when an ESPDMXConfigPacket is received
	 * @param cpacket
	 */
	
	public void configPacketReceived(ESPDMXConfigPacket cpacket) {
		configList.addESPDMX(cpacket);
		_jbGetInfo.setEnabled(true);
	}


	public void sendQuery() {
		LXESPDMXQuerySender nina = new LXESPDMXQuerySender();
		Thread runner = new Thread ( nina );
		runner.start();
	}

	/**
	 * constructs a query packet and tells discoverer to send it on its run loop
	 * @param ipstr
	 * @param udpport
	 */
	public void sendQueryPacket(String ipstr, int udpport, boolean wait) {
		DatagramPacket send_packet = null;
		byte[] packet_buffer = new byte[10];
		
		try {
			InetAddress to_ip = InetAddress.getByName(ipstr);
			send_packet = new DatagramPacket(packet_buffer, 9, to_ip, udpport);
		} catch ( Exception e) {
			System.out.println("query address exception " + e);
			return;
		}

		setStringInPacketBuffer(packet_buffer, "ESP-DMX", 0);
		packet_buffer[8] = OPCODE_QUERY;

		discoverer.setSendPacket(send_packet);	//is sent on next trip through run loop
		
		if ( wait ) {
			try {
				Thread.sleep(1000);   //delay 1 sec. to wait for send/answer
			} catch (Exception e) { }
		}
	}
	
	/**
	 * constructs a reset packet and tells discoverer to send it on its run loop
	 * @param ipstr
	 * @param udpport
	 */
	public void sendResetPacket(String ipstr, int udpport) {
		int result = JOptionPane.showConfirmDialog(
						this,
						"Reset will interrupt wireless connection and DMX.", "Are you sure?",
						JOptionPane.YES_NO_OPTION);
		if (result == JOptionPane.YES_OPTION) {
DatagramPacket send_packet = null;
			byte[] packet_buffer = new byte[10];
			
			try {
				InetAddress to_ip = InetAddress.getByName(ipstr);
				send_packet = new DatagramPacket(packet_buffer, 9, to_ip, udpport);
			} catch ( Exception e) {
				System.out.println("query address exception " + e);
				return;
			}
	
			setStringInPacketBuffer(packet_buffer, "ESP-DMX", 0);
			packet_buffer[8] = OPCODE_RESET;
	
			discoverer.setSendPacket(send_packet);	//is sent on next trip through run loop
		}
	}
	
	// ******************  upload  ******************

	public int upload() {

		String uploadSSID = _jtfSSID.getText();
		if ( uploadSSID == null ) {
			return -2;
		}

		if ( uploadSSID.length() == 0 ) {
			if ( _jrbStation.isSelected() ) {
				alert("Enter SSID for desired network");
				return 2;
			} else {
				uploadSSID = "ESP-DMX";  //default for access point
			}
		}

		String uploadPWD = _jtfPWD.getText();
		if ( uploadPWD == null ) {
			return -2;
		}

		if ( (( uploadPWD.startsWith("****") ) || ( uploadPWD.length() == 0 )) && _jrbStation.isSelected() ) {
			alert("Enter password before uploading");
			return 2;
		}

		String uploadNodeName = _jtfArtNetNodeName.getText();
		if ( uploadNodeName == null ) {
			return -2;
		}
		
		//  *********************  done checking user input, compose packet to send
		
		ESPDMXConfigPacket cp = new ESPDMXConfigPacket(OPCODE_UPLOAD);
		cp.setSSID(uploadSSID);
		cp.setPassword(uploadPWD);
		cp.setNodeName(uploadNodeName);

		uploadPWD = null;
		uploadSSID = null;
		
		cp.setStationMode(_jrbStation.isSelected());
		cp.setArtNetMode(_jrbArtNet.isSelected());
		cp.setDHCPMode(_jrbDHCP.isSelected());
		cp.setMulticastMode(_jrbMulticast.isSelected());
		cp.setRDMMode(_jrbRDM.isSelected());
		cp.setOutputMode(_jrbDMXOutput.isSelected());
		
		cp.setAPStaticAddress(_jtfAccessPointIP.getText());
		cp.setAPGatewayAddress(_jtfAccessPointGateway.getText());
		cp.setAPSubnetAddress(_jtfAccessPointSubnet.getText());
		cp.setStationStaticAddress(_jtfStationIP.getText());
		cp.setStationGatewayAddress(_jtfStationGateway.getText());
		cp.setStationSubnetAddress(_jtfStationSubnet.getText());
		cp.setMulticastAddress(_jtfMulticastAddress.getText());
		cp.setInputAddress(_jtfInputNetTarget.getText());
		int devaddr = 0;
		try {
			devaddr = Integer.parseInt(_jtfDeviceAddress.getText());
		} catch (Exception e) {}
		cp.setDeviceAddress(devaddr);
		
		cp.setSACNUniverse(_jtfSACNUniverse.getText());
		cp.setArtNetPortAddressNet(_jtfArtNetPANet.getText());
		cp.setArtNetSubnet(_jtfArtNetSubnet.getText());
		cp.setArtNetUniverse(_jtfArtNetUniverse.getText());
		
		DatagramPacket send_packet;
		String ipstr = targetIP();
		int udpport = getTargetUDPPort();
		try {
			InetAddress to_ip = InetAddress.getByName(ipstr);
			send_packet = new DatagramPacket(cp.config_data, ESP_CONFIG_PKT_SIZE, to_ip, udpport);
		} catch ( Exception e) {
			System.out.println("upload address exception " + e);
			return -1;
		}
		
		discoverer.reportSendFailure = true;
		discoverer.setSendPacket(send_packet);	//is sent on next trip through run loop
		return 0;
	}
	
	// ************************************   socket utilities  ************************************ 

	public int getTargetUDPPort() {
		int udpport = 0x1936;
		if ( _jrbTargetSACN.isSelected() ) {
			udpport = 0x15C0;
		}
		return udpport;
	}

	public String targetIP() {
		String rs = (String) _jcbTargetSelect.getSelectedItem();
		if ( rs.equals("Default Access Point")) {
			rs = "10.110.115.10";
		}
		return rs;
	}

	// ****************** ******************  packet utilities  ****************** ****************** 

	/**
	 * read 4 byte IP address from an array of bytes
	 * @param data  array of bytes
	 * @param index starting index
	 * @return InetAddress or null
	 */
	public  static InetAddress ipaddressFromByteArray(byte[] data, int index) {
		InetAddress rv = null;
		if ( index <= (data.length - 4)) {
			byte[] ba = new byte[4];
			ba[0] = data[index];
			ba[1] = data[index+1];
			ba[2] = data[index+2];
			ba[3] = data[index+3];
			try {
				rv = InetAddress.getByAddress(ba);
			} catch (Exception e) { }
		}
		return rv;
	}
	
	/**
	 * finds the next zero (\0 string terminator) in an array of bytes
	 * @param data array of bytes
	 * @param index starting index
	 * @return index of \0 (equal to data.length if unterminated)
	 */
	public static int lengthToZeroInByteArray(byte[] data, int index) {
		int c = 0;
		int ci = index;
		while (( data[ci] != 0 ) && (ci< data.length)) {
			c++;
			ci++;
		}
		return c;
	}

	/**
	 * converts a string representation of an IP address into bytes and copies it into array of bytes
	 * @param packet_buffer	array of bytes 
	 * @param ipstr	the string representing an IP address
	 * @param index the starting index in the array of bytes
	 */
	public static void setIPAddressInPacketBuffer(byte[] packet_buffer, String ipstr, int index) {
		InetAddress ipa = null;
		try {
			ipa = InetAddress.getByName(ipstr);
		} catch (Exception e) {

		}
		if ( ( ipa == null ) || ( index > (packet_buffer.length - 4)) ) {
			return;
		}
		byte[] ba = ipa.getAddress();
		packet_buffer[index] = ba[0];
		packet_buffer[index+1] = ba[1];
		packet_buffer[index+2] = ba[2];
		packet_buffer[index+3] = ba[3];
	}

	/**
	 * copies the characters of a string into an array of bytes (including a terminating \0)
	 * @param packet_buffer the array of bytes
	 * @param astr	the string to copy
	 * @param index the starting index in the array
	 */
	public static void setStringInPacketBuffer(byte[] packet_buffer, String astr, int index) {
		byte[] sb = astr.getBytes();
		int max_length = Math.min(astr.length(), packet_buffer.length-index-1);
		int k = 0;
		for ( ; k<max_length; k++) {
			packet_buffer[index+k] = sb[k];
		}
		packet_buffer[index+k] = 0;
	}

	/**
	 * Sends an Art-Net ArtAddress packet containing a command such as Cancel Merge
	 * @param target IP Address to send the command to
	 * @param command	byte representing the command (see Art-Net protocol reference pg 33)
	 * Art-NetTM Designed by and Copyright Artistic Licence Holdings Ltd.
	 */
	public void sendArtAddressCommand (String target, int command) {
		System.out.println("sendArtAddressCommand");
		byte[] artbytes = new byte[107];
		for(int k=0; k<107; k++) {
			artbytes[k] = 0;
		}
		artbytes[0] = 'A';
		artbytes[1] = 'r';
		artbytes[2] = 't';
		artbytes[3] = '-';
		artbytes[4] = 'N';
		artbytes[5] = 'e';
		artbytes[6] = 't';

		artbytes[9] = 0x60;  //opcode
		artbytes[11] = 14;   //protocol version
		artbytes[106] = (byte) command;

		DatagramPacket send_packet;
		
		try {
			InetAddress to_ip = InetAddress.getByName(target);
			send_packet = new DatagramPacket(artbytes, 107, to_ip, 0x1936);
		} catch ( Exception e) {
			System.out.println("art address exception " + e);
			return;
		}

		discoverer.setSendPacket(send_packet);
	}


	// ************************************   other utilities  ************************************ 

	/**
	 * safeParseInt
	 * @param str
	 * @return integer value of string or zero if not a string representing an integer number
	 */
	public static int safeParseInt(String str) {
		int rv = 0;
		try {
			rv = Integer.parseInt(str);
		} catch (Exception e) {
		}
		return rv;
	}

	/**
	 * Display a message in a dialog box.
	 * @param astr message to user
	 */
	public void alert(String astr) {
		JOptionPane.showMessageDialog( null, astr );
	}
	
	/**
	 * send the System.out stream to a log file in the user's Documents directory
	 */
	public void redirectSystemOut() {
		if ( SYSTEM_OUT2LOG ) {
			String path = System.getProperty("user.home") + "/Documents/ESPDMXConfigLog.txt";
	
			try {
				FileOutputStream fos = new FileOutputStream(path);
				PrintStream ps = new PrintStream(fos);
				System.setOut(ps);
				System.setErr(ps);
			} catch (java.io.FileNotFoundException fx) {
				JOptionPane.showMessageDialog(this, "An error occured:\n" + fx );
				System.out.println("exception redirecting stdout " + path);
			}
	
			String jversion = System.getProperty("java.version");
			System.out.print("ESPDMXConfig v1.0.0, java version:" + jversion + "\n");
		}
	}
	
	// ************************************   discovery  ************************************ 
	
	/**
	 * Factory method to create an LXESPDMXDiscoverer
	 * @param networkInterface if not null, searches for an InetAddress associated with the named interface (eg "en0")
	 * @param networkAddress specific IP address to use for binding socket.  Can be null if networkInterface is specified
	 * @return created instance of LXESPDMXDiscoverer (null if socket could not be opened)
	 * 
	 * Searches through available network interfaces for an IP address to bind the LXESPDMXDiscoverer's socket.
     * First, tries to match the name of the interface, eg. en1 is likely wireless on OS X.
     * Next, uses the supplied IP address if no matching interface is found.
     * 
	 */
	
	public LXESPDMXDiscoverer createDiscoverer(String networkInterface, String networkAddress, int port) {
		LXESPDMXDiscoverer christopher = null;
		String myNetworkAddress = networkAddress;
		String searchResults = null;
		String possibleInterface = null;

		// look through available NetworkInterfaces for one matching networkInterface
		// then, list its addresses leaving myNetworkAddress set to the last one found
		// repeats if not found and adds interfaces to searchResults string.
		// keeps track of an interface (other than loopback) that has an IPAddress in possibleInterface
		boolean searchDone = false;
		boolean searched = false;
		while ( ! searchDone ) {
			boolean printNames = searched;
			boolean dosearch = printNames;
			try {
		      Enumeration<NetworkInterface> nets = NetworkInterface.getNetworkInterfaces();
		      while ( nets.hasMoreElements() ) {
		        NetworkInterface nic = nets.nextElement();
		        List<InterfaceAddress> interfaceAddresses = nic.getInterfaceAddresses();
		        if ( printNames ) {
		        	if ( interfaceAddresses.size() > 0 ) {
		        		searchResults = searchResults + nic.getName()+ " ";
		        	}
		        } else {
		        	dosearch = nic.getName().equals(networkInterface);
		        }
		        if ( dosearch) {
		          Iterator<InterfaceAddress>inetAddresses = interfaceAddresses.iterator();
		          while ( inetAddresses.hasNext() ) {
		        	InterfaceAddress addr = inetAddresses.next();
		            String astr = addr.getAddress().getHostAddress();
		            if ( astr.indexOf(":") < 0 ) {      //IPv6 addresses contain colons, skip them
		            	if ( printNames ) {				//printNames is true if no interface was found on first trip through the list
		            		searchResults = searchResults + " " + astr + "\n";
		            		if ( ! astr.equals("127.0.0.1") ) {		
		            			possibleInterface = nic.getName();		// if not loopback, save name of interface as possibility
		            		}
		            	} else {
			            	myNetworkAddress = astr;
							System.out.println("Found address for " + networkInterface + " => " + myNetworkAddress);
			            	searchDone = true;
			            }
		            }
		          }
		        }
		      }
		    } catch (Exception e) {
		    	searchResults ="Warning: exception thrown while searching network interfaces.";
		    }
			if ( ! searchDone ) {
				if ( searched ) {		// second time complete, exit search while loop.
					searchDone = true;
				} else {				// first time complete, start searchResults string and try again
					searched = true;
					searchResults = networkInterface + " not found.  Did find: ";
				}
			}
		}
		
		if ( searchResults != null ) {		// did not find interface so inform user of other possibilities
			alert(searchResults);
			System.out.println(searchResults);
			if ( possibleInterface != null ) {
				_jtfNetInterface.setText(possibleInterface);
				System.out.println("possibly use " + possibleInterface + "?");
			}
		}
		  
		// create the LXESPDMXDiscoverer and a UDP network socket for it, binding the address determined above
	    try {
	      christopher = this.new LXESPDMXDiscoverer();
	      christopher.udpsocket = new DatagramSocket( null );
	      christopher.udpsocket.setReuseAddress(true);
	         if ( myNetworkAddress.equals("0.0.0.0") ) {
	        	 christopher.udpsocket.bind(new InetSocketAddress(InetAddress.getByName("0.0.0.0"), port));
	         } else {
	        	 InetAddress nicAddress = InetAddress.getByName(myNetworkAddress);
	        	 christopher.udpsocket.bind(new InetSocketAddress(nicAddress, port));
	         }
	         christopher.udpsocket.setSoTimeout(1000);
	         christopher.udpsocket.setBroadcast(true);
	   }  catch ( Exception e) {
	      System.out.println("Can't open socket for discovery " + e);
	      christopher = null;
	   }

	    return christopher;
	}
	
	/**
	 * Creates the separate thread for listening and sending UDP packets using discoverer's run loop
	 */
	public void startDiscovery() {
		String networkInterface = _jtfNetInterface.getText();
		String networkAddress = _jtfInterfaceAddress.getText();
		if ( networkAddress.equals("any") ) {
			networkAddress = "0.0.0.0";
		}
		
		discoverer = createDiscoverer(networkInterface, networkAddress, this.getTargetUDPPort());
		if ( discoverer != null ) {
			configList.reset();
			Thread runner = new Thread ( discoverer );
			runner.start();
		}
	}
	
	// ******************  inner classes  ******************
	
	/**
	 * Checks incoming bytes to see if they contain valid config data
     * and packages into ESPDMXConfigPacket object.
	 * @param inpacket source array of bytes
	 * @param ip source IP address
	 * @return ESPDMXConfigPacket or null if inpacket does not contain ESP-DMX config data
	 */
	public static ESPDMXConfigPacket createESPDMXConfigPacket(byte[] inpacket, InetAddress ip) {
		if ( inpacket.length >= ESP_CONFIG_PKT_SIZE ) {
			String header = new String( inpacket, 0, 7 );
			if ( header.equals("ESP-DMX") ) {
				if ( inpacket[8] == OPCODE_DATA ) {
					return new ESPDMXConfigPacket(inpacket, ip);
				}
			}
		}
		return null;
	}
	
	/**
	 * ESPDMXConfigPacket encapsulates an array of bytes and a source IP address
	 * provides getter/setter methods for fields in the ESP-DMX config packet structure
	 */
	public static class ESPDMXConfigPacket {
		
		public static final int ESP_CONFIG_PKT_INDEX_IDENT = 0;
		public static final int ESP_CONFIG_PKT_INDEX_Opcode = 8;
		public static final int ESP_CONFIG_PKT_INDEX_Version = 9;
		public static final int ESP_CONFIG_PKT_INDEX_WiFiMode = 10;
		public static final int ESP_CONFIG_PKT_INDEX_FLAGS = 11;
		public static final int ESP_CONFIG_PKT_INDEX_SSID = 12;
		public static final int ESP_CONFIG_PKT_INDEX_PWD = 76;
		public static final int ESP_CONFIG_PKT_INDEX_APIP = 140;
		public static final int ESP_CONFIG_PKT_INDEX_APGW = 144;
		public static final int ESP_CONFIG_PKT_INDEX_APSN = 148;
		public static final int ESP_CONFIG_PKT_INDEX_STIP = 152;
		public static final int ESP_CONFIG_PKT_INDEX_STGW = 156;
		public static final int ESP_CONFIG_PKT_INDEX_STSN = 160;
		public static final int ESP_CONFIG_PKT_INDEX_MCIP = 164;
		public static final int ESP_CONFIG_PKT_INDEX_SACNU = 168;
		public static final int ESP_CONFIG_PKT_INDEX_ANPAH = 169;
		public static final int ESP_CONFIG_PKT_INDEX_ANPAL = 170;
		public static final int ESP_CONFIG_PKT_INDEX_NODEN = 172;
		public static final int ESP_CONFIG_PKT_INDEX_INIP = 204;
		public static final int ESP_CONFIG_PKT_INDEX_ADDR_LSB = 208;	//note byte order in c struct
		public static final int ESP_CONFIG_PKT_INDEX_ADDR_MSB = 209;
		
		byte[] config_data = new byte[ESP_CONFIG_PKT_SIZE];
		InetAddress sourceip;
		
		public ESPDMXConfigPacket(byte opcode) {
			for (int k=0; k<ESP_CONFIG_PKT_SIZE; k++) {
				config_data[k] = 0;
			}
			setStringInPacketBuffer(config_data, "ESP-DMX", 0);
			config_data[8] = opcode;
		}
		
		public ESPDMXConfigPacket(byte[] data, InetAddress ip) {
			sourceip = ip;
			for (int i=0; i<config_data.length; i++) {
				if ( i < data.length ) {
					config_data[i] = data[i];
				} else {
					config_data[i] = 0;
				}
			}
		}
		
		public InetAddress source() {
			return sourceip;
		}
		
		public String ssid() {
			return new String( config_data, ESP_CONFIG_PKT_INDEX_SSID, lengthToZeroInByteArray(config_data, ESP_CONFIG_PKT_INDEX_SSID) );
		}
		
		public void setSSID(String s) {
			setStringInPacketBuffer(config_data, s, ESP_CONFIG_PKT_INDEX_SSID);		//note no length checking
		}
		
		public String password() {
			return new String( config_data, ESP_CONFIG_PKT_INDEX_PWD, lengthToZeroInByteArray(config_data,ESP_CONFIG_PKT_INDEX_PWD) );
		}
		
		public void setPassword(String p) {
			setStringInPacketBuffer(config_data, p, ESP_CONFIG_PKT_INDEX_PWD);	//note no length checking
		}
		
		public String nodeName() {
			return new String( config_data, ESP_CONFIG_PKT_INDEX_NODEN, lengthToZeroInByteArray(config_data,ESP_CONFIG_PKT_INDEX_NODEN) );
		}
		
		public void setNodeName(String nn) {
			setStringInPacketBuffer(config_data, nn, ESP_CONFIG_PKT_INDEX_NODEN);	//note no length checking
		}
		
		public boolean stationMode() {
			return ( config_data[ESP_CONFIG_PKT_INDEX_WiFiMode] == 0 );
		}
		
		public void setStationMode(boolean m) {
			if ( m ) {
				config_data[ESP_CONFIG_PKT_INDEX_WiFiMode] = 0;
			} else {
				config_data[ESP_CONFIG_PKT_INDEX_WiFiMode] = 1;
			}
		}
		
		public boolean artNetMode() {
			return ( (config_data[ESP_CONFIG_PKT_INDEX_FLAGS] & 1) == 0 );
		}
		
		public void setArtNetMode(boolean m) {
			byte cd = (byte) (config_data[ESP_CONFIG_PKT_INDEX_FLAGS] & 0xfe);
			if ( m ) {
				config_data[ESP_CONFIG_PKT_INDEX_FLAGS] = cd;
			} else {
				config_data[ESP_CONFIG_PKT_INDEX_FLAGS] = (byte) (cd | 0x01);
			}
		}
		
		public boolean dhcpMode() {
			return ( (config_data[ESP_CONFIG_PKT_INDEX_FLAGS] & 2) == 0 );
		}
		
		public void setDHCPMode(boolean m) {
			byte cd = (byte) (config_data[ESP_CONFIG_PKT_INDEX_FLAGS] & 0xfd);
			if ( m ) {
				config_data[ESP_CONFIG_PKT_INDEX_FLAGS] = cd;
			} else {
				config_data[ESP_CONFIG_PKT_INDEX_FLAGS] = (byte) (cd | 0x02);
			}
		}
		
		public boolean multicastMode() {
			return ( (config_data[ESP_CONFIG_PKT_INDEX_FLAGS] & 4) != 0 );
		}
		
		public void setMulticastMode(boolean m) {
			byte cd = (byte) (config_data[ESP_CONFIG_PKT_INDEX_FLAGS] & 0xfb);
			if ( m ) {
				config_data[ESP_CONFIG_PKT_INDEX_FLAGS] = (byte) (cd | 0x04);
			} else {
				config_data[ESP_CONFIG_PKT_INDEX_FLAGS] = cd;
			}
		}
		
		public boolean rdmMode() {
			return ( (config_data[ESP_CONFIG_PKT_INDEX_FLAGS] & 16) != 0 );
		}
		
		public void setRDMMode(boolean m) {
			byte cd = (byte) (config_data[ESP_CONFIG_PKT_INDEX_FLAGS] & 0xef);
			if ( m ) {
				config_data[ESP_CONFIG_PKT_INDEX_FLAGS] = (byte) (cd | 0x10);
			} else {
				config_data[ESP_CONFIG_PKT_INDEX_FLAGS] = cd;
			}
		}
		
		public boolean outputMode() {
			return ( (config_data[ESP_CONFIG_PKT_INDEX_FLAGS] & 8) == 0 );
		}
		
		public void setOutputMode(boolean m) {
			byte cd = (byte) (config_data[ESP_CONFIG_PKT_INDEX_FLAGS] & 0xf7);
			if ( m ) {
				config_data[ESP_CONFIG_PKT_INDEX_FLAGS] = cd;
			} else {
				config_data[ESP_CONFIG_PKT_INDEX_FLAGS] = (byte) (cd | 0x08);
			}
		}
		
		public String apStaticAddress() {
			InetAddress ipa = ipaddressFromByteArray(config_data,ESP_CONFIG_PKT_INDEX_APIP);
			return ipa.getHostAddress();
		}
		
		public void setAPStaticAddress(String ips) {
			setIPAddressInPacketBuffer(config_data, ips, ESP_CONFIG_PKT_INDEX_APIP);
		}
		
		public String apGatewayAddress() {
			InetAddress ipa = ipaddressFromByteArray(config_data,ESP_CONFIG_PKT_INDEX_APGW);
			return ipa.getHostAddress();
		}
		
		public void setAPGatewayAddress(String ips) {
			setIPAddressInPacketBuffer(config_data, ips, ESP_CONFIG_PKT_INDEX_APGW);
		}
		
		public String apSubnetAddress() {
			InetAddress ipa = ipaddressFromByteArray(config_data,ESP_CONFIG_PKT_INDEX_APSN);
			return ipa.getHostAddress();
		}
		
		public void setAPSubnetAddress(String ips) {
			setIPAddressInPacketBuffer(config_data, ips, ESP_CONFIG_PKT_INDEX_APSN);
		}
		
		public String stationStaticAddress() {
			InetAddress ipa = ipaddressFromByteArray(config_data,ESP_CONFIG_PKT_INDEX_STIP);
			return ipa.getHostAddress();
		}
		
		public void setStationStaticAddress(String ips) {
			setIPAddressInPacketBuffer(config_data, ips, ESP_CONFIG_PKT_INDEX_STIP);
		}
		
		public String stationGatewayAddress() {
			InetAddress ipa = ipaddressFromByteArray(config_data,ESP_CONFIG_PKT_INDEX_STGW);
			return ipa.getHostAddress();
		}
		
		public void setStationGatewayAddress(String ips) {
			setIPAddressInPacketBuffer(config_data, ips, ESP_CONFIG_PKT_INDEX_STGW);
		}
		
		public String stationSubnetAddress() {
			InetAddress ipa = ipaddressFromByteArray(config_data,ESP_CONFIG_PKT_INDEX_STSN);
			return ipa.getHostAddress();
		}
		
		public void setStationSubnetAddress(String ips) {
			setIPAddressInPacketBuffer(config_data, ips, ESP_CONFIG_PKT_INDEX_STSN);
		}
		
		public String multicastAddress() {
			InetAddress ipa = ipaddressFromByteArray(config_data,ESP_CONFIG_PKT_INDEX_MCIP);
			return ipa.getHostAddress();
		}
		
		public void setMulticastAddress(String ips) {
			setIPAddressInPacketBuffer(config_data, ips, ESP_CONFIG_PKT_INDEX_MCIP);
		}
		
		public String inputAddress() {
			InetAddress ipa = ipaddressFromByteArray(config_data,ESP_CONFIG_PKT_INDEX_INIP);
			return ipa.getHostAddress();
		}
		
		public void setInputAddress(String ips) {
			setIPAddressInPacketBuffer(config_data, ips, ESP_CONFIG_PKT_INDEX_INIP);
		}
		
		public int deviceAddress() {
			return ((config_data[ESP_CONFIG_PKT_INDEX_ADDR_MSB]&0xff) << 8) + (config_data[ESP_CONFIG_PKT_INDEX_ADDR_LSB]&0xff);
		}
		
		public void setDeviceAddress(int d) {
			config_data[ESP_CONFIG_PKT_INDEX_ADDR_MSB] = (byte)((d >> 8) & 0xff);
			config_data[ESP_CONFIG_PKT_INDEX_ADDR_LSB] = (byte)(d & 0xff);
		}
		
		public String sacnUniverse() {
			return Integer.toString(config_data[ESP_CONFIG_PKT_INDEX_SACNU] | (config_data[ESP_CONFIG_PKT_INDEX_SACNU+3]<<8));
		}
		
		public void setSACNUniverse(String s) {
			int su = safeParseInt(s);
			config_data[ESP_CONFIG_PKT_INDEX_SACNU] = (byte)( su & 0xff);
			config_data[ESP_CONFIG_PKT_INDEX_SACNU+3] = (byte)( (su >> 8) & 0xff);
		}
		
		public String artNetPortAddressNet() {
			return Integer.toString(config_data[ESP_CONFIG_PKT_INDEX_ANPAH]);
		}
		
		public void setArtNetPortAddressNet(String n) {
			config_data[ESP_CONFIG_PKT_INDEX_ANPAH] = (byte)safeParseInt(n);
		}
		
		public String artNetSubnet() {
			return Integer.toString((config_data[ESP_CONFIG_PKT_INDEX_ANPAL] >> 4) & 0x0f);
		}
		
		public void setArtNetSubnet(String s) {
			int sn = (byte)safeParseInt(s);
			int pal = config_data[ESP_CONFIG_PKT_INDEX_ANPAL];
			config_data[ESP_CONFIG_PKT_INDEX_ANPAL] = (byte)((pal & 0x0f) | ((sn &0x0f) << 4));
		}
		
		public String artNetUniverse() {
			return Integer.toString(config_data[ESP_CONFIG_PKT_INDEX_ANPAL] & 0x0f);
		}
		
		public void setArtNetUniverse(String s) {
			int u = (byte)safeParseInt(s);
			int pal = config_data[ESP_CONFIG_PKT_INDEX_ANPAL];
			config_data[ESP_CONFIG_PKT_INDEX_ANPAL] =(byte)((pal & 0xf0) | (u & 0x0f));
		}
		
	}	// class ESPDMXConfigPacket
	
	
	/**
	 * ESPDMXConfigPacketList is a static inner class that serves a dual purpose
	 * as a way of storing ESPDMXConfigPacket objects that have been received
	 * and as a data model for a table listing these packets.
	 */
	public static class ESPDMXConfigPacketList extends AbstractTableModel {
		private static final long serialVersionUID = 1L;
		Vector<ESPDMXConfigPacket> cpackets;
	
		public ESPDMXConfigPacketList() {
			cpackets = new Vector<ESPDMXConfigPacket>();
		}
		
		/**
		 * packetAt
		 * @param index of packet in list
		 * @return ESPDMXConfigPacket at index
		 */
		public ESPDMXConfigPacket packetAt(int index) {
			return cpackets.elementAt(index);
		}
		
		/**
		 * packetMatchingAddress
		 * Search to see if a packet in the list has been received from a particular sender
		 * @param ip InetAddress to match
		 * @return ESPDMXConfigPacket that has been received from InetAddress ip.
		 */
		public ESPDMXConfigPacket packetMatchingAddress(InetAddress ip) {
			Enumeration<ESPDMXConfigPacket> en = cpackets.elements();
			ESPDMXConfigPacket ccp;
			while ( en.hasMoreElements() ) {
				ccp = en.nextElement();
				if ( ccp.source().equals(ip)) {
					return ccp;
				}
			}
			return null;
		}
		
		/**
		 * addESPDMX
		 * @param cpacket ESPDMXConfigPacket
		 *  Check to see if a packet has already been received from the sender of
		 *  cpacket.  If so, remove the previous packet.
		 *  Then either way, add cpacket to the list of received configs.
		 */
		public void addESPDMX(ESPDMXConfigPacket cpacket) {
			ESPDMXConfigPacket ep = packetMatchingAddress(cpacket.source());
			if ( ep != null ) {
				cpackets.remove(ep);
			}
			cpackets.add(cpacket);
			fireTableDataChanged();
		}
		
		/**
		 * foundConfig
		 * @return true if at least one config packet has been received
		 */
		public boolean foundConfig() {
			return ( cpackets.size() > 0 );
		}
		
		/**
		 * Clear the list of prior configs
		 */
		public void reset() {
			cpackets.removeAllElements();
		}
		
		// ********************  Implementation of TableModel interface  ********************
		
		  public int getRowCount() {
		    return cpackets.size();
		  }

		  public int getColumnCount() {
		    return 2;
		  }

		  public Object getValueAt(int row, int column) {
			ESPDMXConfigPacket cp = cpackets.elementAt(row);
			if ( column == 0 ) {
				return cp.source().getHostAddress();
			}
			return cp.nodeName();
		  }

		  public Class<String> getColumnClass(int column) {
		    return String.class;
		  }

		  public String getColumnName(int column) {
		    if ( column == 0 ) {
		    	return "IP Address";
		    }
		    return "Name";
		  }

		  public boolean isCellEditable(int row, int column) {
		    return false;
		  }
	}		//class ESPDMXConfigPacketList
	
	/**
	 * LXESPDMXDiscoverer is an inner class that allows listening for and sending
	 * packets on a separate thread.
	 */
	class LXESPDMXDiscoverer extends Object implements Runnable  {
		
		/**
		 *  A socket for network communication
		 */
		DatagramSocket udpsocket = null;
		
		/**
		 *  When set to a DatagramPacket, will be sent in run loop
		 */
		public DatagramPacket sendPacket = null;
		
		/**
		 *  true while run loop is active
		 */
		public boolean reportSendFailure = false;
		
		/**
		 *  true while run loop is active
		 */
		public boolean searching;
		
		/**
		 * buffer for reading and sending packets
		 */
		byte[] _packet_buffer = new byte[1024];

		
		public LXESPDMXDiscoverer() {
			
		}
		
		/**
		  * The main loop checks to see if a packet is waiting to be sent. (and sends it)
		  * If there's no outgoing, it tries to receive a packet.  If one is received and
		  * it is a ESP-DMX packet, it is bundled into a ESPDMXConfigPacket object which
		  * is passed to the outer class' configPacketReceived method.
		 */
		public void run() {
			searching = true;
			int exceptionCount = 0;
			
			while ( searching ) {
				if ( sendPacket != null ) {
					try {
						udpsocket.send(sendPacket);
						sendPacket = null;
						exceptionCount = 0;
					} catch (Exception e) {
						exceptionCount++;
						if ( reportSendFailure ) {
							JOptionPane.showMessageDialog( null, "Packet failed to send. " + e );
						}
						System.out.println("send packet exception " + e);
						if ( exceptionCount > 3 ) {
							sendPacket = null;
						}
						reportSendFailure = false;
					}
				} else {
					receivePacket(udpsocket);//may timeout
				}
			}
			
			udpsocket.close();
			udpsocket = null;
		}
		
		public boolean receivePacket(DatagramSocket socket) {
			boolean result = false;
			DatagramPacket receivePacket = new DatagramPacket(_packet_buffer, _packet_buffer.length);
			try {
				socket.receive(receivePacket);
				if ( receivePacket.getLength() >= ESP_CONFIG_PKT_SIZE ) {
					ESPDMXConfigPacket cpacket = createESPDMXConfigPacket(_packet_buffer, receivePacket.getAddress());
	
					if ( cpacket != null ) {
						configPacketReceived(cpacket);
						result = true;
					}
				}
			} catch ( Exception e) {
				//   will catch receive time out exception
				//System.out.println("receive exception " + e);
			}
			return result;
		}
		
		public void setSendPacket(DatagramPacket p) {
			while ( sendPacket != null ) {
				try {
					Thread.sleep(1000);			//wait for previous packet to be sent
				} catch (Exception e) {
					return;						//sleep interrupted, don't loop forever
				}
			}
			sendPacket = p;
		}
		
		/**
		 * stops the search and closes the udpsocket if it exists
		 */
		public void close() {
			if ( searching ) {
				searching = false;	// setting this to false will cause run() loop to exit, closing udpsocket
			} else if ( udpsocket != null ) {
				udpsocket.close();
				udpsocket = null;
			}
		}
		
	}	//LXESPDMXDiscoverer class
	
	/**
	 * LXESPDMXQuerySender is inner class allowing query packets to be sent on a separate thread.
	 *	Using a separate thread allows the interface to be updated along side the query function.
	 *  First a query packet is sent to the specified target address.
	 *  Then, query packets are also sent to other common broadcast addresses.
	 */
	
	class LXESPDMXQuerySender extends Object implements Runnable  {
		
		public LXESPDMXQuerySender() {
		}
		
		public void run() {
			int udpport = getTargetUDPPort();
			String utarget = targetIP();
			
			String progress = "searching: " + utarget;
			_jLabelProgress.setText(progress);
			sendQueryPacket(utarget, udpport, true);

			// also send query packet to other addresses
			String starget = null;
			int i=0;
			progress = "searching";
			while (i<7) {
				switch (i) {
				case 0:
					starget = "10.110.115.10";
					break;
				case 1:
					starget = "10.110.115.255";
					break;
				case 2:
					starget = "10.255.255.255";
					break;
				case 3:
					starget = "192.168.1.1";
					break;
				case 4:
					starget = "192.168.1.255";
					break;
				case 5:
					starget = "255.255.255.255";
					break;
				case 6:
					starget = _jtfMulticastAddress.getText();
					if ( starget.length() == 0 ) {
						starget = "239.255.0.1";
					}
					break;
				}
				if ( ! utarget.equals(starget) ) {
					_jLabelProgress.setText(progress);
					progress = progress + ".";
					sendQueryPacket(starget, udpport, true);
				}
				i++;
			}  //while
			_jLabelProgress.setText("");
		}
	}

// ******************************************************************************************
// ******************  static main & run methods  *******************************************
	
	public static void Run (String clarg) {
		try {
			//comment out this to remove native look and feel
			try {
				UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
			} 
			catch (Exception e) { 
			}
			@SuppressWarnings("unused")			
			ESPDMXConfig frame;
			
			if ( clarg == null ) {
				frame = new ESPDMXConfig();
			} else {
				frame = new ESPDMXConfig(clarg);
			}
		}
		catch (Exception e) {
			JOptionPane.showMessageDialog(null, "An app error occured:\n" + e );
		}
	}

	// Main entry point
	static public void main(String[] args) {
		if ( args.length > 0 ) {
			Run(args[0]);
		} else {
			Run(null);
		}
	}
	
}