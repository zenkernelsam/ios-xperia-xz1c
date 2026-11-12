import Foundation
import AppKit

guard CommandLine.arguments.count == 3 else { fatalError("usage: recycle_macos plan.json journal.json") }
let planURL = URL(fileURLWithPath: CommandLine.arguments[1])
let journalURL = URL(fileURLWithPath: CommandLine.arguments[2])
let plan = try JSONSerialization.jsonObject(with: Data(contentsOf: planURL)) as! [String: Any]
let roots = plan["allowed_roots"] as! [String]
let items = plan["items"] as! [[String: String]]
var results = [[String: String]]()
for item in items {
    let source = URL(fileURLWithPath: item["path"]!).standardizedFileURL
    guard roots.contains(where: { source.path.hasPrefix($0 + "/") }),
          FileManager.default.fileExists(atPath: source.path),
          ![".git", "dxb", "research", "flash", "unlock", "backups"].contains(source.lastPathComponent)
    else { fatalError("Source outside approved workspace or protected: \(source.path)") }
    var finished = false
    var failure: Error?
    var destination: URL?
    NSWorkspace.shared.recycle([source]) { mapping, error in
        failure = error
        destination = mapping[source]
        finished = true
    }
    let limit = Date().addingTimeInterval(120)
    while !finished && Date() < limit {
        RunLoop.current.run(until: Date().addingTimeInterval(0.05))
    }
    guard finished, failure == nil, let target = destination,
          !FileManager.default.fileExists(atPath: source.path),
          FileManager.default.fileExists(atPath: target.path)
    else { fatalError("Native Trash failed: \(String(describing: failure))") }
    results.append(["source": source.path, "trash_path": target.path, "native_trash_verified": "true"])
    let bytes = try JSONSerialization.data(withJSONObject: results, options: [.prettyPrinted, .sortedKeys])
    try bytes.write(to: journalURL, options: .atomic)
}
print(String(data: try JSONSerialization.data(withJSONObject: results, options: [.sortedKeys]), encoding: .utf8)!)
