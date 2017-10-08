//
//  DetailViewController.swift
//  Controller
//
//  Created by Joël Gähwiler on 08.10.17.
//  Copyright © 2017 Through Momentum. All rights reserved.
//

import UIKit

let lightWidth: Double = 10
let lightLength: Double = 200
let objectWidth: Double = 100
let floorWidth: Double = 300
let floorHeight: Double = 3
let bottomPadding: Double = 50

class DetailViewController: UIViewController {
    var mainVC: MainViewController?
    
    var ropeView: UIView?
    var lightView: UIView?
    var floorView: UIView?
    var objectView: UIView?
    
    @IBOutlet var idLabel: UILabel?
    
    var id = 0
    var position: Double = 100
    var distance: Double = 100
    var motion: Bool = false
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        // write id
        idLabel!.text = String(format: "%02d", id)
        
        // create rope view
        ropeView = UIView()
        ropeView!.layer.borderWidth = 1
        ropeView!.layer.borderColor = UIColor.white.cgColor
        view.addSubview(ropeView!)
        
        // create light view
        lightView = UIView()
        lightView!.backgroundColor = UIColor.white
        view.addSubview(lightView!)
        
        // create floor view
        floorView = UIView()
        floorView!.layer.borderWidth = 3
        floorView!.layer.borderColor = UIColor.white.cgColor
        view.addSubview(floorView!)
        
        // create object view
        objectView = UIView()
        objectView!.backgroundColor = UIColor.black
        view.addSubview(objectView!)
        
        // recalculate frames
        recalculate()
    }
    
    func recalculate() {
        // calculate dimensions
        let fw = Double(view.frame.width)
        let fh = Double(view.frame.height)
        let objectHeight = position-distance
        let ropeLength = fh-bottomPadding-objectHeight-distance-lightLength
        
        // set frames
        ropeView!.frame = CGRect(x: fw/2, y: 0, width: 1, height: ropeLength)
        lightView!.frame = CGRect(x: fw/2 - lightWidth/2, y: ropeLength, width: lightWidth, height: lightLength)
        floorView!.frame = CGRect(x: fw/2 - floorWidth/2, y: fh-bottomPadding, width: floorWidth, height: floorHeight)
        objectView!.frame = CGRect(x: fw/2 - objectWidth/2, y: fh-bottomPadding-objectHeight, width: objectWidth, height: objectHeight)
    }
    
    @IBAction func stop() {
        send(topic: "stop", payload: "")
    }
    
    @IBAction func automateOn() {
        send(topic: "naos/set/automate", payload: "on")
    }
    
    @IBAction func automateOff() {
        send(topic: "naos/set/automate", payload: "off")
    }
    
    @IBAction func turnUp() {
        send(topic: "turn", payload: "up")
    }
    
    @IBAction func turnDown() {
        send(topic: "turn", payload: "down")
    }
    
    @IBAction func reset100() {
        send(topic: "reset", payload: "100")
    }
    
    @IBAction func flash() {
        send(topic: "flash", payload: "500")
    }
    
    @IBAction func disco() {
        send(topic: "disco", payload: "")
    }
    
    @IBAction func back(_ sender: Any) {
        dismiss(animated: true, completion: nil)
    }
    
    // Helpers
    
    func send(topic: String, payload: String) {
        // send message using the main view controller
        if let mvc = mainVC {
            mvc.send(id: id, topic: topic, payload: payload)
        }
    }
    
    // UIViewController
    
    override var prefersStatusBarHidden: Bool {
        return true
    }
}