import javax.swing.*;

import java.awt.*;
import java.awt.geom.*;
import java.awt.event.*;
import java.net.*;


public class ESPDMXConfig extends JFrame {

	private static final long serialVersionUID = 1L;

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

	JTextField _jtfArtNetSubnet;
	JTextField _jtfArtNetUniverse;
	JTextField _jtfArtNetNodeName;
	
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
	ButtonGroup _bgDMXMode;
	JRadioButton _jrbDMXOutput;
	JRadioButton _jrbDMXInput;

	ButtonGroup _bgTargetPort;
	JRadioButton _jrbTargetSACN;
	JRadioButton _jrbTargetArtNet;

	JComboBox<String> _jcbTargetSelect;

	JButton _jbClearOutput;
	JButton _jbCancelMerge;
	JButton _jbGetInfo;
	JButton _jbUpload;
	
	Line2D _line1;
	Line2D _line2;
	Line2D _line3;
	ImageIcon _image1 = null;

	byte[] packet_buffer = new byte[256];
	
	public static byte OPCODE_DATA = 0;
	public static byte OPCODE_QUERY = '?';
	public static byte OPCODE_UPLOAD = '!';
	
	public ESPDMXConfig() {
		initInterface();
	}

	public ESPDMXConfig(String args) {
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
		resizeAndCenter(670,600,this);

		addWindowListener(new java.awt.event.WindowAdapter() {
			public void windowClosing(java.awt.event.WindowEvent e) {
				exit();
			}
		});

		setLayout(null);

		_jtfSSID = new javax.swing.JTextField();
		_jtfSSID.setSize(new java.awt.Dimension(275, 25));
		_jtfSSID.setText( "" );
		_jtfSSID.setLocation(new java.awt.Point(110, 10));
		add(_jtfSSID);

		javax.swing.JLabel jLabel1 = new javax.swing.JLabel();
		jLabel1.setSize(new java.awt.Dimension(80, 20));
		jLabel1.setVisible(true);
		jLabel1.setText("SSID:");
		jLabel1.setHorizontalAlignment(SwingConstants.RIGHT);
		jLabel1.setLocation(new java.awt.Point(28, 12));
		add(jLabel1);

		_jtfPWD = new javax.swing.JTextField();
		_jtfPWD.setSize(new java.awt.Dimension(275, 25));
		_jtfPWD.setText( "" );
		_jtfPWD.setLocation(new java.awt.Point(110, 40));
		add(_jtfPWD);

		javax.swing.JLabel jLabel2 = new javax.swing.JLabel();
		jLabel2.setSize(new java.awt.Dimension(80, 20));
		jLabel2.setVisible(true);
		jLabel2.setText("Password:");
		jLabel2.setHorizontalAlignment(SwingConstants.RIGHT);
		jLabel2.setLocation(new java.awt.Point(28, 42));
		add(jLabel2);

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
		add(_jrbAccessPoint);
		add(_jrbStation);

		// ap fields & labels

		_jtfAccessPointIP = new javax.swing.JTextField();
		_jtfAccessPointIP.setSize(new java.awt.Dimension(130, 25));
		_jtfAccessPointIP.setText( "" );
		_jtfAccessPointIP.setLocation(new java.awt.Point(110, 130));
		add(_jtfAccessPointIP);

		javax.swing.JLabel jLabel3 = new javax.swing.JLabel();
		jLabel3.setSize(new java.awt.Dimension(80, 20));
		jLabel3.setVisible(true);
		jLabel3.setText("AP Address:");
		jLabel3.setHorizontalAlignment(SwingConstants.RIGHT);
		jLabel3.setLocation(new java.awt.Point(28, 132));
		add(jLabel3);

		_jtfAccessPointGateway = new javax.swing.JTextField();
		_jtfAccessPointGateway.setSize(new java.awt.Dimension(130, 25));
		_jtfAccessPointGateway.setText( "" );
		_jtfAccessPointGateway.setLocation(new java.awt.Point(110, 160));
		add(_jtfAccessPointGateway);

		javax.swing.JLabel jLabel4 = new javax.swing.JLabel();
		jLabel4.setSize(new java.awt.Dimension(80, 20));
		jLabel4.setVisible(true);
		jLabel4.setText("AP Gateway:");
		jLabel4.setHorizontalAlignment(SwingConstants.RIGHT);
		jLabel4.setLocation(new java.awt.Point(28, 162));
		add(jLabel4);

		_jtfAccessPointSubnet = new javax.swing.JTextField();
		_jtfAccessPointSubnet.setSize(new java.awt.Dimension(130, 25));
		_jtfAccessPointSubnet.setText( "" );
		_jtfAccessPointSubnet.setLocation(new java.awt.Point(110, 190));
		add(_jtfAccessPointSubnet);

		javax.swing.JLabel jLabel5 = new javax.swing.JLabel();
		jLabel5.setSize(new java.awt.Dimension(80, 20));
		jLabel5.setVisible(true);
		jLabel5.setText("AP Subnet:");
		jLabel5.setHorizontalAlignment(SwingConstants.RIGHT);
		jLabel5.setLocation(new java.awt.Point(28, 192));
		add(jLabel5);

		// station fields & labels
		
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
		add(_jrbStatic);
		add(_jrbDHCP);

		_jtfStationIP = new javax.swing.JTextField();
		_jtfStationIP.setSize(new java.awt.Dimension(130, 25));
		_jtfStationIP.setText( "" );
		_jtfStationIP.setLocation(new java.awt.Point(375, 130));
		add(_jtfStationIP);

		javax.swing.JLabel jLabel6 = new javax.swing.JLabel();
		jLabel6.setSize(new java.awt.Dimension(80, 20));
		jLabel6.setVisible(true);
		jLabel6.setText("Address:");
		jLabel6.setHorizontalAlignment(SwingConstants.RIGHT);
		jLabel6.setLocation(new java.awt.Point(292, 132));
		add(jLabel6);

		_jtfStationGateway = new javax.swing.JTextField();
		_jtfStationGateway.setSize(new java.awt.Dimension(130, 25));
		_jtfStationGateway.setText( "" );
		_jtfStationGateway.setLocation(new java.awt.Point(375, 160));
		add(_jtfStationGateway);

		javax.swing.JLabel jLabel7 = new javax.swing.JLabel();
		jLabel7.setSize(new java.awt.Dimension(80, 20));
		jLabel7.setVisible(true);
		jLabel7.setText("Gateway:");
		jLabel7.setHorizontalAlignment(SwingConstants.RIGHT);
		jLabel7.setLocation(new java.awt.Point(292, 162));
		add(jLabel7);

		_jtfStationSubnet = new javax.swing.JTextField();
		_jtfStationSubnet.setSize(new java.awt.Dimension(130, 25));
		_jtfStationSubnet.setText( "" );
		_jtfStationSubnet.setLocation(new java.awt.Point(375, 190));
		add(_jtfStationSubnet);

		javax.swing.JLabel jLabel8 = new javax.swing.JLabel();
		jLabel8.setSize(new java.awt.Dimension(80, 20));
		jLabel8.setVisible(true);
		jLabel8.setText("Subnet:");
		jLabel8.setHorizontalAlignment(SwingConstants.RIGHT);
		jLabel8.setLocation(new java.awt.Point(292, 192));
		add(jLabel8);

		_line1 = new Line2D.Float(30, 250, 620, 250);
		
		// protocol options

		_jrbSACN = new JRadioButton("sACN");
		_jrbSACN.setLocation(new Point(130, 245));
		_jrbSACN.setSize(new Dimension(200, 20));
		_jrbSACN.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				_jrbMulticast.setSelected(true);
			}
		});

		_jrbArtNet = new JRadioButton("Art-Net");
		_jrbArtNet.setLocation(new Point(375, 245));
		_jrbArtNet.setSize(new Dimension(200, 20));
		_jrbArtNet.setSelected(true);
		_jrbArtNet.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				_jrbBroadcast.setSelected(true);
			}
		});

		_bgProtocol = new ButtonGroup();
		_bgProtocol.add(_jrbSACN);
		_bgProtocol.add(_jrbArtNet);
		add(_jrbSACN);
		add(_jrbArtNet);

		// sacn text fields

		_jtfMulticastAddress = new javax.swing.JTextField();
		_jtfMulticastAddress.setSize(new java.awt.Dimension(130, 25));
		_jtfMulticastAddress.setText( "" );
		_jtfMulticastAddress.setLocation(new java.awt.Point(110, 280));
		add(_jtfMulticastAddress);

		javax.swing.JLabel jLabel9 = new javax.swing.JLabel();
		jLabel9.setSize(new java.awt.Dimension(80, 20));
		jLabel9.setVisible(true);
		jLabel9.setText("Multicast IP:");
		jLabel9.setHorizontalAlignment(SwingConstants.RIGHT);
		jLabel9.setLocation(new java.awt.Point(28, 282));
		add(jLabel9);

		_jtfSACNUniverse = new javax.swing.JTextField();
		_jtfSACNUniverse.setSize(new java.awt.Dimension(40, 25));
		_jtfSACNUniverse.setText( "" );
		_jtfSACNUniverse.setLocation(new java.awt.Point(110, 310));
		add(_jtfSACNUniverse);

		javax.swing.JLabel jLabel10 = new javax.swing.JLabel();
		jLabel10.setSize(new java.awt.Dimension(100, 20));
		jLabel10.setVisible(true);
		jLabel10.setText("sACN Universe:");
		jLabel10.setHorizontalAlignment(SwingConstants.RIGHT);
		jLabel10.setLocation(new java.awt.Point(8, 312));
		add(jLabel10);

		// Art-Net text fields

		_jtfArtNetSubnet = new javax.swing.JTextField();
		_jtfArtNetSubnet.setSize(new java.awt.Dimension(40, 25));
		_jtfArtNetSubnet.setText( "" );
		_jtfArtNetSubnet.setLocation(new java.awt.Point(375, 280));
		add(_jtfArtNetSubnet);

		javax.swing.JLabel jLabel11 = new javax.swing.JLabel();
		jLabel11.setSize(new java.awt.Dimension(110, 20));
		jLabel11.setVisible(true);
		jLabel11.setText("Art-Net Subnet:");
		jLabel11.setHorizontalAlignment(SwingConstants.RIGHT);
		jLabel11.setLocation(new java.awt.Point(262, 282));
		add(jLabel11);

		_jtfArtNetUniverse = new javax.swing.JTextField();
		_jtfArtNetUniverse.setSize(new java.awt.Dimension(40, 25));
		_jtfArtNetUniverse.setText( "" );
		_jtfArtNetUniverse.setLocation(new java.awt.Point(375, 310));
		add(_jtfArtNetUniverse);

		javax.swing.JLabel jLabel12 = new javax.swing.JLabel();
		jLabel12.setSize(new java.awt.Dimension(120, 20));
		jLabel12.setVisible(true);
		jLabel12.setText("Art-Net Universe:");
		jLabel12.setHorizontalAlignment(SwingConstants.RIGHT);
		jLabel12.setLocation(new java.awt.Point(252, 312));
		add(jLabel12);

		_jtfArtNetNodeName = new javax.swing.JTextField();
		_jtfArtNetNodeName.setSize(new java.awt.Dimension(275, 25));
		_jtfArtNetNodeName.setText( "" );
		_jtfArtNetNodeName.setLocation(new java.awt.Point(375, 340));
		add(_jtfArtNetNodeName);

		javax.swing.JLabel jLabel13 = new javax.swing.JLabel();
		jLabel13.setSize(new java.awt.Dimension(120, 20));
		jLabel13.setVisible(true);
		jLabel13.setText("Node Name:");
		jLabel13.setHorizontalAlignment(SwingConstants.RIGHT);
		jLabel13.setLocation(new java.awt.Point(252, 342));
		add(jLabel13);
		
		_jbClearOutput = new JButton("Clear Output");
		_jbClearOutput.setSize(new Dimension(120, 25));
		_jbClearOutput.setLocation(new Point(530, 310));
		_jbClearOutput.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				sendArtAddressCommand(targetIP(), 0x90);
			}
		});
		add(_jbClearOutput);

		_jbCancelMerge = new JButton("Cancel Merge");
		_jbCancelMerge.setSize(new Dimension(120, 25));
		_jbCancelMerge.setLocation(new Point(530, 280));
		_jbCancelMerge.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				sendArtAddressCommand(targetIP(), 0x01);
			}
		});
		add(_jbCancelMerge);
		
		_line2 = new Line2D.Float(30, 400, 620, 400);
		
		_jrbMulticast = new JRadioButton("Multicast");
		_jrbMulticast.setLocation(new Point(25, 395));
		_jrbMulticast.setSize(new Dimension(200, 20));

		_jrbBroadcast = new JRadioButton("Unicast/Broadcast");
		_jrbBroadcast.setLocation(new Point(25, 425));
		_jrbBroadcast.setSize(new Dimension(200, 20));
		_jrbBroadcast.setSelected(true);

		_bgMultiBroadcast = new ButtonGroup();
		_bgMultiBroadcast.add(_jrbMulticast);
		_bgMultiBroadcast.add(_jrbBroadcast);
		add(_jrbMulticast);
		add(_jrbBroadcast);
		
		_jrbDMXOutput = new JRadioButton("DMX Output");
		_jrbDMXOutput.setLocation(new Point(210, 395));
		_jrbDMXOutput.setSize(new Dimension(150, 20));
		_jrbDMXOutput.setSelected(true);

		_jrbDMXInput = new JRadioButton("DMX Input");
		_jrbDMXInput.setLocation(new Point(210, 425));
		_jrbDMXInput.setSize(new Dimension(160, 20));

		_bgDMXMode = new ButtonGroup();
		_bgDMXMode.add(_jrbDMXOutput);
		_bgDMXMode.add(_jrbDMXInput);
		add(_jrbDMXOutput);
		add(_jrbDMXInput);
		
		
		_jtfInputNetTarget = new javax.swing.JTextField();
		_jtfInputNetTarget.setSize(new java.awt.Dimension(130, 25));
		_jtfInputNetTarget.setText( "" );
		_jtfInputNetTarget.setLocation(new java.awt.Point(375, 423));
		add(_jtfInputNetTarget);

		javax.swing.JLabel jLabel16 = new javax.swing.JLabel();
		jLabel16.setSize(new java.awt.Dimension(120, 20));
		jLabel16.setVisible(true);
		jLabel16.setText("to net:");
		jLabel16.setHorizontalAlignment(SwingConstants.RIGHT);
		jLabel16.setLocation(new java.awt.Point(252, 425));
		add(jLabel16);

		// target stuff
		
		_line3 = new Line2D.Float(10, 490, 640, 490);

		_jcbTargetSelect = new JComboBox<String>();
		_jcbTargetSelect.addItem("Default Access Point");
		_jcbTargetSelect.addItem("10.110.115.255");
		_jcbTargetSelect.addItem("10.255.255.255");
		_jcbTargetSelect.addItem("192.168.1.255");
		_jcbTargetSelect.setSize(new Dimension(200, 25));
		_jcbTargetSelect.setLocation(new Point(110, 490));
		_jcbTargetSelect.setEditable(true);
		add(_jcbTargetSelect);

		javax.swing.JLabel jLabel14 = new javax.swing.JLabel();
		jLabel14.setSize(new java.awt.Dimension(80, 20));
		jLabel14.setVisible(true);
		jLabel14.setText("Target IP:");
		jLabel14.setHorizontalAlignment(SwingConstants.RIGHT);
		jLabel14.setLocation(new java.awt.Point(28, 492));
		add(jLabel14);

		_jrbTargetSACN = new JRadioButton("sACN");
		_jrbTargetSACN.setLocation(new Point(192, 525));
		_jrbTargetSACN.setSize(new Dimension(90, 20));

		_jrbTargetArtNet = new JRadioButton("Art-Net");
		_jrbTargetArtNet.setLocation(new Point(107, 525));
		_jrbTargetArtNet.setSize(new Dimension(100, 20));
		_jrbTargetArtNet.setSelected(true);
		
		javax.swing.JLabel jLabel15 = new javax.swing.JLabel();
		jLabel15.setSize(new java.awt.Dimension(80, 20));
		jLabel15.setVisible(true);
		jLabel15.setText("Target Port:");
		jLabel15.setHorizontalAlignment(SwingConstants.RIGHT);
		jLabel15.setLocation(new java.awt.Point(28, 525));
		add(jLabel15);

		_bgTargetPort = new ButtonGroup();
		_bgTargetPort.add(_jrbTargetSACN);
		_bgTargetPort.add(_jrbTargetArtNet);
		add(_jrbTargetSACN);
		add(_jrbTargetArtNet);

		_jbGetInfo = new JButton("Get Info");
		_jbGetInfo.setSize(new Dimension(120, 30));
		_jbGetInfo.setLocation(new Point(375, 490));
		_jbGetInfo.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				sendQuery();
			}
		});
		add(_jbGetInfo);

		_jbUpload = new JButton("Upload");
		_jbUpload.setSize(new Dimension(120, 30));
		_jbUpload.setLocation(new Point(375, 525));
		_jbUpload.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				int result = upload();
				if ( result < 0 ) {
					alert("Upload failed");
				}
			}
		});
		add(_jbUpload);
		
		_image1 = new ImageIcon(getClass().getResource("ESP_DMX_1.png"));

		setVisible(true);
	}

	public void exit() {
		System.out.print("Exited " + new java.util.Date());
		System.exit(0);
	}
	
	public void paint(Graphics g) {
        super.paint(g);
        ((Graphics2D) g).draw(_line1);
        ((Graphics2D) g).draw(_line2);
        ((Graphics2D) g).draw(_line3);
        if ( _image1 != null ) {
        	_image1.paintIcon(this, g, 525, 512);
        }
    }
	
	// ******************  query & receive  ******************

	public void readPacket() {
		_jtfSSID.setText(new String( packet_buffer, 9, lengthToZeroInPacketBuffer(9) ));
		_jtfPWD.setText(new String( packet_buffer, 73, lengthToZeroInPacketBuffer(73) ));
		_jtfArtNetNodeName.setText(new String( packet_buffer, 171, lengthToZeroInPacketBuffer(171) ));

		if ( packet_buffer[137] == 0 ) {
			_jrbStation.setSelected(true);
		} else {
			_jrbAccessPoint.setSelected(true);
		}

		if ( (packet_buffer[138] & 1) == 0 ) {
			_jrbArtNet.setSelected(true);
		} else {
			_jrbSACN.setSelected(true);
		}

		if ( (packet_buffer[138] & 2) == 0 ) {
			_jrbDHCP.setSelected(true);
		} else {
			_jrbStatic.setSelected(true);
		}

		if ( (packet_buffer[138] & 4) == 0 ) {
			_jrbBroadcast.setSelected(true);
		} else {
			_jrbMulticast.setSelected(true);
		}
		
		if ( (packet_buffer[138] & 8) == 0 ) {
			_jrbDMXOutput.setSelected(true);
		} else {
			_jrbDMXInput.setSelected(true);
		}

		InetAddress ipa = ipaddressFromPacketBuffer(140);
		_jtfAccessPointIP.setText(ipa.getHostAddress());

		ipa = ipaddressFromPacketBuffer(144);
		_jtfAccessPointGateway.setText(ipa.getHostAddress());

		ipa = ipaddressFromPacketBuffer(148);
		_jtfAccessPointSubnet.setText(ipa.getHostAddress());

		ipa = ipaddressFromPacketBuffer(152);
		_jtfStationIP.setText(ipa.getHostAddress());

		ipa = ipaddressFromPacketBuffer(156);
		_jtfStationGateway.setText(ipa.getHostAddress());

		ipa = ipaddressFromPacketBuffer(160);
		_jtfStationSubnet.setText(ipa.getHostAddress());

		ipa = ipaddressFromPacketBuffer(164);
		_jtfMulticastAddress.setText(ipa.getHostAddress());
		
		ipa = ipaddressFromPacketBuffer(204);
		_jtfInputNetTarget.setText(ipa.getHostAddress());

		_jtfSACNUniverse.setText(Integer.toString(packet_buffer[168]));
		_jtfArtNetSubnet.setText(Integer.toString(packet_buffer[169]));
		_jtfArtNetUniverse.setText(Integer.toString(packet_buffer[170]));
	}

	public boolean receivePacket(DatagramSocket socket) {
		boolean result = false;
		DatagramPacket receivePacket = new DatagramPacket(packet_buffer, packet_buffer.length);
		try {
			socket.receive(receivePacket);
			
			String header = new String( packet_buffer, 0, 7 );
			if ( header.equals("ESP-DMX") ) {
				if ( packet_buffer[8] == OPCODE_DATA ) {
					readPacket();
					InetAddress ipa = receivePacket.getAddress();
					_jcbTargetSelect.setSelectedItem(ipa.getHostAddress());
					result = true;
				}
			}
		} catch ( Exception e) {
			//   will catch receive time out exception
			//System.out.println("receive exception " + e);
		}
		return result;
	}

	public void sendQuery() {
		int udpport = getTargetUDPPort();
		DatagramSocket socket = initSocket(udpport);
		if ( socket == null ) {
			alert("No network socket.");
			return;
		}

		boolean found = false;
		String utarget = targetIP();
		found = sendQueryPacketAndListen(socket, utarget, udpport);

		if ( ! found ) {                //if not found, try other targets
			String starget = null;
			int i=0;
			while ((i<4) && ! found) {
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
					starget = "192.168.1.255";
					break;
				}
				if ( ! utarget.equals(starget) ) {
					System.out.println("Searching " + starget);
					found = sendQueryPacketAndListen(socket, starget, udpport);
				}
				i++;
			}  //while ! found && i<4
		}

		if ( found ) {
			alert("Found ESP-DMX!");
		} else {
			alert("Did not find an ESP-DMX." );
		}
		socket.close();
	}

	public boolean sendQueryPacketAndListen(DatagramSocket socket, String ipstr, int udpport) {
		DatagramPacket sendPacket;

		try {
			InetAddress to_ip = InetAddress.getByName(ipstr);
			sendPacket = new DatagramPacket(packet_buffer, 9, to_ip, udpport);
		} catch ( Exception e) {
			System.out.println("query address exception " + e);
			return false;
		}

		setStringInPacketBuffer("ESP-DMX", 0);
		packet_buffer[8] = OPCODE_QUERY;

		boolean sentQuery = true;
		boolean receivedReply = false;

		try {
			socket.send(sendPacket);
		} catch ( Exception e) {
			System.out.println("send packet exception " + e);
			sentQuery = false;
		}

		if ( sentQuery ) {
			receivedReply = true;
			if ( ! receivePacket(socket) ) {
				if ( ! receivePacket(socket) ) {
					if ( ! receivePacket(socket) ) {
						receivedReply = false;
					}
				}
			}
		}

		return receivedReply;
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

		int udpport = getTargetUDPPort();
		DatagramSocket socket = initSocket(udpport);
		if ( socket == null ) {
			return -1;
		}

		for (int k=0; k<204; k++) {
			packet_buffer[k] = 0;
		}

		setStringInPacketBuffer("ESP-DMX", 0);
		packet_buffer[8] = OPCODE_UPLOAD;

		setStringInPacketBuffer(uploadPWD, 73);
		setStringInPacketBuffer(uploadSSID, 9);
		setStringInPacketBuffer(uploadNodeName, 171);
		System.out.println("_uploading " +uploadSSID+ ", " + uploadPWD+", "+uploadNodeName);
		uploadPWD = null;
		uploadSSID = null;

		if ( _jrbAccessPoint.isSelected() ) {
			packet_buffer[137] = 1;  //access point
		} else {
			packet_buffer[137] = 0;
		}
		packet_buffer[138] = 0;
		if ( _jrbSACN.isSelected() ) {
			packet_buffer[138] += 1;  //sacn
		}
		if ( _jrbStatic.isSelected() ) {
			packet_buffer[138] += 2;  //static
		}
		if ( _jrbMulticast.isSelected() ) {
			packet_buffer[138] += 4;  //multicast
		}
		if ( _jrbDMXInput.isSelected() ) {
			packet_buffer[138] += 8;  //input mode
		}
		
		setIPAddressInPacketBuffer(_jtfAccessPointIP.getText(), 140);
		setIPAddressInPacketBuffer(_jtfAccessPointGateway.getText(), 144);
		setIPAddressInPacketBuffer(_jtfAccessPointSubnet.getText(), 148);
		setIPAddressInPacketBuffer(_jtfStationIP.getText(), 152);
		setIPAddressInPacketBuffer(_jtfStationGateway.getText(), 156);
		setIPAddressInPacketBuffer(_jtfStationSubnet.getText(), 160);
		setIPAddressInPacketBuffer(_jtfMulticastAddress.getText(), 164);
		setIPAddressInPacketBuffer(_jtfInputNetTarget.getText(), 204);

		packet_buffer[139] = 2;  //wifi channel not implemented
		packet_buffer[168] = (byte)safeParseInt(_jtfSACNUniverse.getText());
		packet_buffer[169] = (byte)safeParseInt(_jtfArtNetSubnet.getText());
		packet_buffer[170] = (byte)safeParseInt(_jtfArtNetUniverse.getText());
		

		DatagramPacket sendPacket;
		String ipstr = targetIP();
		try {
			InetAddress to_ip = InetAddress.getByName(ipstr);
			sendPacket = new DatagramPacket(packet_buffer, 208, to_ip, udpport);
		} catch ( Exception e) {
			System.out.println("upload address exception " + e);
			return -1;
		}

		boolean sentPacket = true;
		try {
			socket.send(sendPacket);
		} catch ( Exception e) {
			System.out.println("send packet exception " + e);
			sentPacket = false;
		}
		if ( sentPacket ) {
			if ( sendQueryPacketAndListen(socket, ipstr, udpport) ) {
				alert("Upload confirmation received.");
			} else {
				alert("Did not confirm upload." );
			}
		}

		socket.close();
		return 0;
	}
	
	// ******************  socket utilities  ******************

	public DatagramSocket initSocket(int port) {
		DatagramSocket socket;

		try {
			socket = new DatagramSocket( null );
			socket.setReuseAddress(true);
			socket.bind(new InetSocketAddress(InetAddress.getByName("0.0.0.0"), port));
			socket.setSoTimeout(1000);
			socket.setBroadcast(true);
		} catch ( Exception e) {
			System.out.println("socket exception " + e);
			return null;
		}
		return socket;
	}

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

	// ******************  packet utilities  ******************

	public InetAddress ipaddressFromPacketBuffer(int index) {
		byte[] ba = new byte[4];
		ba[0] = packet_buffer[index];
		ba[1] = packet_buffer[index+1];
		ba[2] = packet_buffer[index+2];
		ba[3] = packet_buffer[index+3];
		InetAddress rv = null;
		try {
			rv = InetAddress.getByAddress(ba);
		} catch (Exception e) {

		}
		return rv;
	}

	public void setIPAddressInPacketBuffer(String ipstr, int index) {
		InetAddress ipa = null;
		try {
			ipa = InetAddress.getByName(ipstr);
		} catch (Exception e) {

		}
		if ( ipa == null ) {
			return;
		}
		byte[] ba = ipa.getAddress();
		packet_buffer[index] = ba[0];
		packet_buffer[index+1] = ba[1];
		packet_buffer[index+2] = ba[2];
		packet_buffer[index+3] = ba[3];
	}

	public void setStringInPacketBuffer(String astr, int index) {
		byte[] sb = astr.getBytes();
		for (int k=0; k<astr.length(); k++) {
			packet_buffer[index+k] = sb[k];
		}
	}

	public int lengthToZeroInPacketBuffer(int index) {
		int c = 0;
		int ci = index;
		while ( packet_buffer[ci] != 0 ) {
			c++;
			ci++;
		}
		return c;
	}

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

		DatagramSocket socket = initSocket(0x1936);
		if ( socket == null ) {
			return ;
		}

		try {
			InetAddress to_ip = InetAddress.getByName(target);
			DatagramPacket sendPacket = new DatagramPacket(artbytes, 107, to_ip, 0x1936);
			socket.send(sendPacket);
		} catch ( Exception e) {
			System.out.println("art address exception " + e);
		}

		socket.close();
	}


	// ******************  other utilities  ******************

	public int safeParseInt(String str) {
		int rv = 0;
		try {
			rv = Integer.parseInt(str);
		} catch (Exception e) {
		}
		return rv;
	}

	public void alert(String astr) {
		JOptionPane.showMessageDialog( null, astr );
	}
	
	// ******************  static run methods  ******************
	
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